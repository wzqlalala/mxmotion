#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mBasicEnum.h"

#include <QString>
#include <QObject>
#include <QVector>
#include <vector>
#include <QFileInfoList>
#include <QFile>
#include <QSet>
#include <QHash>
#include <QList>
#include <QRunnable>
#include <set>
#include <QTextCodec>

#include "mModelStruct.h"
#include "mModelData.h"
#include "mElementFunction.h"
#include "mIOEnum.h"
#include "mPlyLaminate.h"

//贡献量
#include "mModalContriData.h"
#include "mNodeContriData.h"
#include "mPanelContriData.h"

using namespace std;
using namespace MxFunctions;

namespace MechMaterialData
{
    class mMaterialBase;
}

namespace MechContactData
{
    class mContactBase;
    class mPeriodicBoundary;
}

namespace MechModelFixData
{
    class mModelFixManager;
    class mEleSetCtrl;
}

namespace MechStructOptionData
{
    class mOptimizationConstraintData;
}

namespace MechIOFile
{
    class mIMeshThread;
    class mIMxFile : public QObject
    {
        Q_OBJECT
    public:

        ~mIMxFile();

        /*
        * 扫描文件
        */
        void scanMxFile(const QString& filename);

        /*
        * 读取文件
        */
        bool readMxFile(const QString& filename);

        /*
        * 设置该文件类型为网格文件
        */
        void setIsMeshFile();

        /*
        * 读取自定义材料库数据
        */
        std::shared_ptr<MechMaterialData::mMaterialBase> ReadCustomMaterialData(const QString& filename);

    private:
        //读取网格信息
        MxElementMessage GetElementMessage(QString list);

        //筛选单元类型是否不需要读取
        bool isSpecialElement(ElementType type);

        /*
        *截取关键字名称
        */
        QString getKeyValue(const QString& str_line, const QString& key);

        /*
        *截取关键字名称
        */
        QString getKeyValue(const QString& str_line, const QString& key, const QString& interval);

        /*
        * 截取第几个参数
        */
        QString getQStringListIndexValue(const QStringList& list, const int& index);

        /*
         * 读取节点数据（累加）,仅在单场调用
         */
        void ReadNodeData();

        /*
        * 读取单元数据
        */
        void readElementData();

        /*
        * 读取特殊单元
        */
        void readSpecialElement(MxElementMessage mess);

        /*
        * 读取结构优化数据
        */
        void ReadStructOptionData();

        /*
        * 读取分析工况
        */
        void ReadCaseData();

        /*
        * 读取材料数据
        */
        void ReadMaterialData();

        /*
        * 读Hashin失效
        */
        void ReadHashIndamage(shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 读取Hash渐变损伤数据
        */
        void ReadEnergyDamager(shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 读取截面属性数据
        */
        void ReadPropertyData();

        /*
         * 读取集合数据
         */
        void ReadSetData();

        /*
        * 读取非结构质量
        */
        void ReadNonStructQualityData();

        /*
        * 读取离散场
        */
        void readDiscreteField();

        /*
        * 读取分析场（点云/对象离散）
        */
        void readAnalysisField();

        /*
        * 读取接触数据
        */
        void ReadContactData();

        /*
        * 读取周期性边界
        */
        void readPeriodicBoundaryData();

        /*
        * 读取过盈接触
        */
        void ReadContactinterference();

        /*
        * 读取载荷数据
        */
        void ReadLoadData();

        /*
        * 读边界条件
        */
        void readBoundary();

        /*
        * 读坐标系
        */
        void readCoordinatesystem();

        /*
        * 坐标系值按XOY转换
        */
        QStringList getXOY(const QVector3D& xPoint, const QVector3D& origin, const QVector3D& xyPoint) const;

    private:
        /*
        * 设置单元额外的数据
        */
        void setElementExtraData(int elementId, ElementType elementType, QString currentline);

        /*
        * 读分析工况下载荷*load
        */
        void readCaseLoad(const int& caseID);

        /*
        * 读分析工况下转子分析
        */
        void readOmega(const int& caseID);

        /*
        * 该分析工况下非结构质量 *nsm
        */
        void readCaseNsm(const int& caseID);

        /*
        * 读分析工况下边界条件*load
        */
        void readCaseBC(const int& caseID);


        //读模型修改
        void readModelChangeData();

        // 处理 *modelchange 多 set 合并：文件读取完成后生成新的 element set
        void mergeModelChangeSets();

        /*
        * 读初始温度预定义场定义
        */
        void readInitTempData();

        /*
        * 读温度载荷预定义场定义
        */
        void readTempLoadData();

        /*
        * 读分析下初始温度预定义场引用
        */
        void readCaseInitTemp(const int& caseID);

        /*
        * 读分析下温度载荷预定义场引用
        */
        void readCaseTempLoadField(const int& caseID);

        /*
        * 读分析下 初始缺陷
        */
        void readCaseInitBug(const int& caseID);

        /*
        * 读分析下 过盈接触
        */
        void readCaseContactinterference(const int& caseID);

        /*
        * 读分析下模型修复
        */
        void readCaseModelFix(const int& caseID);

        /*
        * 读分析下输出
        */
        void readCaseOutPut(const int& caseID);

        /*
        * 读分析下PCH输出
        */
        void readCasePchOutPut(const int& caseID);

        /*
        * 读全局输出控制
        */
        void readGlobalOutputControl();

        /*
  * 读分析下历史数据输出
  */
        void readHistoryOutput(const int& caseID);

        /*
        * 各向同性线弹性材料
        */
        void readIsolinearelastic(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 二维正交各向异性线弹性
        */
        void readOrtholinearelastic2d(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 三维工程常数各向异性线弹性
        */
        void readOrtholinearelastic3d(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 二维各向异性线弹性
        */
        void readAnisotropiclinearelastic2d(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 三维各向异性线弹性
        */
        void readAnisotropiclinearelastic3d(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        *   热学材料本构
        */
        void readHeatMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        *  chaboche
        */
        void readChabocheMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        *  Johnson-Cook
        */
        void readJohnsonCookMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        *  剪切损伤
        */
        void readShearDamageMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        *  剪切损伤材料
        */
        void readShearDamage(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        *  损伤演化
        */
        void readDamageEvolution(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        *  声学材料
        */
        void readaCousticMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        *  变形塑性材料
        */
        void readDeformationplasticityMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        *  粘塑性材料
        */
        void readViscoplasticityMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        *  粘弹性材料
        */
        void readViscoelasticMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 弹塑性材料（各向同性硬化、随动硬化、混合硬化）
        */
        void readElastoplasticMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 垫片材料
        */
        void readShimMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 超泡沫材料
        */
        void readHyperforMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 读取试验数据
        */
        void ReadTestData(shared_ptr<MechMaterialData::mMaterialBase> mat, const MaterialClass& matType, const MaterialPropType& propType);

        /*
        * Neo-Hookean2
        */
        void readNeoHookean2Mat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * Mooney-Rivlin
        */
        void readMooneyRivlinMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * Arruda-Boyce
        */
        void readArrudaBoyceMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);
        /*
        * yeoh
        */
        void readYeohMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
         * 断裂材料
         */
        void readBreakMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        //断裂材料-相场参数
        void readPhaseParam(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
         * Ogden
         */
        void readOgdenMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
         * Polynomial
         */
        void readPolynomialMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        //TODO:新增七个读
        //读弹性材料 MAT_Flex
        void readFlexMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        //读塑性参数 带阶数 MAT_PlasticityWithOrder
        void readPlasticityWithOrderMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        //读塑性参数 下拉框 MAT_Plasticity
        void readPlasticityComboMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        //读Johnson-Cook塑性参数 MAT_PlasticityJohnson
        void readPlasticityJohnsonMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        //读Johnson-Cook率相关参数 MAT_RateDependent
        void readRateDependentMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        //读厚度行为 MAT_ThicknessBehavior
        void readThicknessBehaviorMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        //读膜行为 MAT_FilmBehavior
        void readFilmBehaviorMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        //读剪切行为 MAT_CutBehavior
        void readCutBehaviorMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        //读循环硬化带表格 MAT_CyclicHardeningWithTable
        void readCyclicHardeningWithTable(std::shared_ptr<MechMaterialData::mMaterialBase> mat);


        /*
        * 密度
        */
        void readDensity(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 声速
        */
        void readSoundVelocity(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 体积模量
        */
        void readBulkModulus(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 流体阻尼系数
        */
        void readAcousticMaterialDamping(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 材料阻尼
        */
        void readMaterialDamping(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 热膨胀系数
        */
        void readExpansion(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 热传导系数、比热容系数
        */
        void readHeat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 读失效应变
        */
        void readFailureEffect(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 读失效应力
        */
        void readFailureStress(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 蠕变行为
        */
        void readCreep(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        *  循环硬化
        */
        void readCyclichardening(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 进度条显示
        */
        void setProgressBarValue(int pos);

        /*
        * 解析映射关系
        */
        void analysisMapRelation();

        /*
        * 导入完成后，对启用了转子动力学的复模态分析工况触发树节点刷新
        */
        void notifyRotorSettingChangedForImportedCases();

        /*
        * 温度相关曲线储存
        */
        QVariant appendTempCurve(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const MaterialClass& matType, const MaterialPropType& propType, int column, const QString& errorName, const QString& dependency, bool flag = false);


        /*
        * 材料曲线结构体储存
        */
        void appendMatPropertyValue(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const MaterialClass& matType, const MaterialPropType& propType, QVector<QVector<double>>& curve);

        /*
        * 材料变量储存
        */
        void appendMatPropertyValue(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const MaterialClass& matType, const MaterialPropType& propType, const QString& value, MechCommon::QVariantType verType = MechCommon::QVariantType::QVariant_Double);

        /*
        * 读杆
        */
        void readTorsionBar(const int& phyID, const QString& propertyname);

        /*
        * 读剪切板
        */
        void readShearplatesection(const int& phyID, const QString& propertyname);

        /*
        * 读管截面
        */
        void readTubesection(const int& phyID, const QString& propertyname);

        /*
        * 读壳单元截面
        */
        void readShellsection(const int& phyID, const QString& propertyname);

        /*
        * 读梁截面
        */
        void readBeamsection(const int& phyID, const QString& propertyname);

        /*
        * 读弹簧截面
        */
        void readSpringsection(const int& phyID, const QString& propertyname);

        /*
        * 读阻尼截面
        */
        void readViscousdampsection(const int& phyID, const QString& propertyname);

        /*
        * 读cbush截面
        */
        void readBushsection(const int& phyID, const QString& propertyname);

        /*
        * 读平面应变截面
        */
        void readPlanestrainsection(const int& phyID, const QString& propertyname);

        /*
        * 读平面应力截面
        */
        void readPlanestresssection(const int& phyID, const QString& propertyname);

        /*
         * 读垫片截面
         */
        void readGasketsection(const int& phyID, const QString& propertyname);

        /*
        * 读间隙截面
        */
        void readGapunisection(const int& phyID, const QString& propertyname);

        /*
        * 读宏观等效截面
        */
        void readMultiscalesection(const int& phyID, const QString& propertyname);

        /*
        * 读铺层
        */
        void readPlyLaminate(const int& phyID, const QString& propertyname);
        void readSolidShell(MechPropertyData::mPlyLaminate* prodata);
        void readSolid(MechPropertyData::mPlyLaminate* prodata);
        void readShell(MechPropertyData::mPlyLaminate* prodata);

        /*
        * 创建截面形状
        */
        int createSectionShape(const QString& secType, vector<double>& xbs, bool isPeami = false);

        /*
        * 集合数据储存
        */
        void readSetData(vector<RenderPickResultData>& ids, QString generate, int maxID, bool isPart = false, const int setID = 0);

        void readDofSetData(vector<RenderPickResultData>& ids, QString generate, int maxID, bool isPart = false);

        void readSurface(vector<RenderPickResultData>& ids, QString generate, int setID);

        void readEdge(vector<RenderPickResultData>& ids, QString generate, int setID);

        void transformMixSet(int setID);

        void appendRenderPickResultData(vector<RenderPickResultData>& ids, const int& result_id, int partID = -1, int result_id2 = -1);

        /*
        * 读点面/线面接触（以 line = on 区分）
        */
        void readNodeToSurfaceContact(MechContactData::mContactBase* contact);

        /*
        * 读后统一回填点面/线面接触 slave_region：用户自建集合卡位于接触卡之后，
        * 读接触时按集合名判定会漏判，映射解析完成后按从面集合真实类型回填
        */
        void rebuildNodeToSurfaceSlaveRegion();

        /*
        * 读面面接触
        */
        void readSurfaceToSurfaceContact(MechContactData::mContactBase* contact);

        /*
        * 读通用接触
        */
        void readCommonContact(MechContactData::mContactBase* contact);

        /*
        * 读自接触
        */
        void readSelfContact(MechContactData::mContactBase* contact);

        /*
        * 法向行为
        */
        void readBehavior(MechContactData::mContactBase* contact);

        /*
        *  接触定义摩擦属性
        */
        void readFriction(MechContactData::mContactBase* contact);

        /*
        *  热接触间隙传导
        */
        void readGapconductance(MechContactData::mContactBase* contact);

        /*
        *  几何调整值
        */
        void readAdjust(MechContactData::mContactBase* contact);

        /*
        * 接触间隙
        */
        void readClearance(MechContactData::mContactBase* contact);

        /*
        * 接触压力均匀性评估指标
        */
        void readEvaluation (MechContactData::mContactBase* contact);

        /*
        * 读集中力与力矩
        */
        void readNodalload();

        /*
        * 读重力
        */
        void readBodyload();

        /*
        * 读表面压力
        */
        void readSurfaceload();

        /*
        * 读表面载荷
        */
        void readSurface();

        /*
        * 读点热流
        */
        void readNodalflux();

        /*
        * 读时域载荷
        */
        void readTransient();

        /*
        * 读频域载荷rload
        */
        void readFreqdomain(QString loadType);

        /*
        * 读组合载荷
        */
        void readCombined();

        /*
        * 读声学载荷
        */
        void readAcousticConcForce();

        /*
        * 读声学频域载荷
        */
        void readAcousticFrequencyDomain();

        /*
        * 读螺栓载荷
        */
        void readBoltload();

        /*
        * 读谱载荷
        */
        void readSpectrum();

        /*
        * 读旋转加速度
        */
        void readRoACC();

        /*
        * 读旋转速度
        */
        void readRoSpeed();

        /*
        * 读边载荷
        */
        void readEdgeload();

        /*
        * 读压强
        */
        void readPressure();

        /*
        * 读热流密度
        */
        void readHeatFlux();

        /*
        * 读热辐射
        */
        void readRadiation();

        /*
        * 读对流换热
        */
        void readConvection();

        /*
        * 读spc
        */
        void readSpc();

        /*
        * 读虚拟约束
        */
        void readSuport();

        /*
        * 读超单元边界
        */
        void readSuperelement();

        /*
        * 读子模型位移边界
        */
        void readSubmodelMove(const QString& name, const QString& set);

        /*
        * 读子模型温度边界
        */
        void readSubmodelTempNew(const QString& name, const QString& set);

        /*
        * 读连接
        */
        void readConnector();

        /*
        * 刚性连接
        */
        void readRigidConnector();

        /*
        * 柔性连接
        */
        void readFlexibleConnector();

        /*
        * 绑定
        */
        void readBind();

        /*
        * mpc
        */
        void readMPC();

        /*
         * 离散刚体
         */
        void readDisperse();

        /*
         * 螺栓约束
         */
        void readPretension();

        /*
        * 读连接器
        */
        void readConnectingDeputy(const QString& propertyname, const QString& coordinatename);

        /*
        * 读轴承单元
        */
        void readBearingSection(const QString& propertyname);

        /*
        * 读fastener焊点
        */
        void readFastener();

        /*
         * 分布式耦合
         */
        void readDistributedCoupling();

        /*
         * 采样频率点
         */
        void readFreqSamplPointData();

        /*
         * 读层合结构
         */
        void readLaminateData();

        /*
        * 线性静态分析
        */
        void readLinearstatic(const QString& casename, int caseID);

        /*
        * 非线性分析
        */
        void readNonlinear(const QString& casename, int caseID);

        /*
        * 传热分析
        */
        void readHeattransfer(const QString& casename, int caseID);

        /*
        * 模态分析
        */
        void readModesComplexmodes(const QString& casename, int caseID);

        /*
        * 线性屈曲
        */
        void readBuckling(const QString& casename, int caseID);

        /*
        * 响应谱分析
        */
        void readResponseSpectrum(const QString& casename, int caseID);

        /*
        * 随机振动分析
        */
        void readRandomVibration(const QString& casename, int caseID);

        /*
        * 解析随机振动分析映射
        */
        void analysisRandomCaseOutPut();

        /*
        * 解析面板与单元集映射
        */
        void analysisPanleElementMap();

        /*
        * 非结构质量与截面属性映射
        */
        void nonPropertyMap(); //(未完成)
        /*
        * 非结构质量与单元映射
        */
        void nonElementMap(); //(未完成)
        /*
        * 非结构质量与单元集合映射
        */
        void nonElementSetMap();

        /*
        * 分析工况与非结构质量映射
        */
        void caseNsmMap();

        /*
        * 频响分析
        */
        void readFrequencyresponse(const QString& casename, int caseID);

        /*
        * 声固耦合分析
        */
        void readASCoupling(const QString& casename, int caseID);

        /*
        * 线性瞬态分析
        */
        void readLineartransient(const QString& casename, int caseID);

        /*
        * 超单元分析
        */
        void readGeneric(const QString& casename, int caseID);

        /*
        /*
        * 读取模态贡献量数据
        */
        void readModalContriData(int caseID, MechGlobalParamData::mModalContriData* data);

        /*
        * 读取节点贡献量数据
        */
        void readNodeContriData(int caseID, MechGlobalParamData::mNodeContriData* data);

        /*
        * 读取面板贡献量数据
        */
        void readPanelContriData(int caseID, MechGlobalParamData::mPanelContriData* data);

        /*
        * 读取传递路径贡献量数据
        */
        void readTPContriData();

        /*
        * 读取结构优化参数数据
        */
        void ReadStructOptionTopopara();

        /*
        * 读取结构优化设计域数据
        */
        void ReadStructOptionDesign();

        /*
        * 读取结构优化约束数据
        */
        void ReadStructOptionConstraint();

        /*
        * 读取结构优化约束-响应类型子块(与优化目标的*response子块格式保持一致)
        */
        void ReadStructOptionConstraintResponse(MechStructOptionData::mOptimizationConstraintData* constraint, const QString& type, const QString& targetstep);

        /*
        * 读结构优化目标数据
        */
        void ReadStructOptionObjective();

        /*
        * 读超单元调用
        */
        void readGenericAssignData();

        /*
        *  读超单元参数
        */
        void readGenericParamData();

        /*
        * 读readParam
        */
        void readParam();

        /*
        * 读声固耦合对搜索
        */
        void readASCouplingSearchData();

        /*
        * 读函数
        */
        void readFunctionData();

        /*
        * 读等效辐射功率
        */
        void readERPData();

        /*
        * 读峰值筛选数据
        */
        void readPeakFilterData();

    private:
        /**************************************储存文件读取时，储存在数据库中的名字****************************************************/
        QString ExistRenamePhysicalName(MxFunctions::PhysicalFieldType type, const QString& name);
        QHash<MxFunctions::PhysicalFieldType, QHash<QString, QString>> _readPhyName;

        void appendMapPhysicalStruct(MxFunctions::PhysicalFieldType phyType1, int id1, MxFunctions::PhysicalFieldType type2, QString name2, MxFunctions::MapRelationType relationType = MapRelation_None);
        void appendMapPhysicalTable(MxFunctions::PhysicalFieldType phyType, int id1, MxFunctions::PhysicalFieldType type2, std::vector<QString> nameList);

        QHash<MxFunctions::PhysicalFieldType, QHash<int, QHash<MxFunctions::PhysicalFieldType, std::vector<QString>>>> _mapVecPhysical;
        QHash<MxFunctions::PhysicalFieldType, QHash<int, QHash<MxFunctions::PhysicalFieldType, QHash<MxFunctions::MapRelationType, QString>>>> _mapPhysicalStruct;
        /* 临时存储铺层厚度离散场名称，用于解析完成后设置 _thickness 下拉框（仅 Shell 类型） */
        QHash<int, QString> _tempPlyThickFieldNames;

        /*
        * 载荷、边界、预定义场、接触、转子分析对应的分析
        */
        QHash<MxFunctions::PhysicalFieldType, QHash<QString, std::vector<std::pair<int, double>>>> _mapCaseValue;
        /*
        * 单元表面/边
        */
        QHash<int, QHash<QString, int>> _elementSurfaceToElement;
        /*
        * 坐标系中节点
        */
        QHash<int, vector<int>> _coorToNodes;
        /*
        * 单元中坐标系
        */
        std::set<int> _elementToCoorName;
        /*
        * 随机振动分析
        */
        struct RandomCase_OutPut
        {
            QString caseName1;
            QString caseName2;
            QString funcName;
        };

        QHash<int, QList<RandomCase_OutPut>> _randomCaseOutPut;

        //面板集
        QHash<int, QList<QString>> _panelToElements; //单元集
        QHash<int, QList<QString>> _panelToNodeSets; //节点集

        //节点区域集合名登记（读*set卡时记录 node/nodesurface，供点面接触从面回填 slave_region 判定节点表面模式用）
        QSet<QString> _nodeSetNames;

        //非结构质量
        QHash<int, QList<QString>> _nonToPropertyName; //截面属性
        QHash<int, QList<int>> _nonToElementId; //单元
        QHash<int, QList<QString>> _nonToSetName; //单元集

        //分析工况
        QHash<int, QList<QString>> _caseNsm; //非结构质量

        /*
        * 螺栓载荷映射
        */
        void boltloadMap();
        //螺栓载荷 
        QHash<int, int> _boltloadMap; //(载荷id, 节点id)
        QHash<int, QString> _boltloadToSet;  //(载荷id, 集合名)
        //螺栓约束 
        QHash<int, int> _pretensionMap; //(节点id, 连接id)
        QHash<QString, int> _SetToPretension; //(集合名, 连接id)


        //过盈接触映射
        void contactMap();
        QHash<std::pair<QString, QString>, int> _contactinterference_hash;
        QHash<std::pair<QString, QString>, int> _contact_hash;

        //参考坐标系与显示坐标系
        void nodeCoordMap();
        std::map<int, std::pair<QString, QString>> _nodeCoordinateData; //节点ID -> <参考坐标系名， 分析坐标系名>


        //当前读取的离散场
        MechDiscreteFieldData::mDiscreteField* _currentDiscreteField = nullptr;

    private:
        //模型数据
        MechModelData::mModelData* _modeldata;

        //网格数据
        mMeshData* _meshData;

        //当前行
        QString currentline{ "" };

        //文件名字
        QString _filename;

        //存储之前的网格最大编号
        int _lastMaxMeshID;

        //存储之前的节点最大编号
        int _lastMaxNodeID;

        //节点连续id
        int _curnodenum;

        //读完单元
        quint64 _elementEndLine;

        //文件大小
        quint64 _fileSize;

        bool _isMeshFile = false;

        //单元对应部件id
        QHash<int, int> _elementIDToPartID;

        QFile _file;
        QStringList _linelist;

        QHash<int, std::vector<QString>> m_NodesurfaceHash;
    };
}
