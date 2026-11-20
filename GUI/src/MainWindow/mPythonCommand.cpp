#include "mPythonCommand.h"
#include "mPostExportAPIRegistrar.h"
#include "mContactAPIRegistrar.h"
#include "mContactManager.h"
#include "mContactBase.h"
#include "mMixSet.h"
#include "mGlobalParam.h"
#include "mBoundaryFixed.h"
#include "mBoundaryTemp.h"
#include "mLoadForce.h"
#include "mLoadConvection.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mSolveTaskCenter.h"
//#include "mGeoCommandSolidOfSketchOCC.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（拉 OCC 链；冻结岛保档，同参照处理）
//include "mGeoCommandFactory.h" //【OCC摘除·S10】毒头（拉 mGeoCommandBase→OCC 链/冻结岛）死引用清剿，同参照处理
#include "mMechGlobalSignals.h"
#include "mLoadBoundaryManager.h"
#include "mMaterialBase.h"
#include "mSolidHomo.h"
#include "mCaseImplicitLinearStatic.h"
#include "mCaseImplicitSteadyHeat.h"
#include "mVariableFunction.h"
#include <mGeoPythonAPIRegistrar.h>
#include <mMeshPythonAPIRegistrar.h>
#include <mCaseImplicitModal.h>
#include <mConnectorBind.h>
#include "mPredefinedFieldInitTemp.h"
#include "mDirectModelingController.h"
#include <QVector>
#include "mxr/view/mBaseRend.h"
#include "MxOptim.h"

using namespace MechContactData;
using namespace MechSetData;
using namespace MechGlobalParamData;
using namespace MGeoCommand;
using namespace MechCommon;
using namespace MechCaseData;
namespace MechMainWindow
{
    namespace
    {
        // 与 mAPIScriptExecutor 的保留参数名保持一致：geometryIdMode="original"
        // 的 JSON 脚本每个步骤都会携带该上下文，用于把“最初导入时的几何 ID”
        // 换算成直接建模之后的当前 ID；Python API 和 current 模式脚本不注入。
        const QString kScriptOriginalGeometryIdContextParam =
            QStringLiteral("__directModelingOriginalFaceIdContext");

        QString scriptOriginalGeometryIdContext(const APIServer::APIParams& params)
        {
            const QStringList values =
                params.value(kScriptOriginalGeometryIdContextParam);
            return values.size() == 1 ? values.front().trimmed() : QString();
        }

        // elementDim 与 DirectModelingController 约定一致：0=点、1=线、2=面、3=体。
        // 上下文为空时保持原值（当前 ID 模式）；非整数 ID 直接报错，避免被
        // toInt() 静默解析成 0 后把物理场绑定到错误对象。
        bool resolveScriptGeometryIds(
            const QString& contextId,
            int elementDim,
            QStringList& ids,
            QString& errorMessage)
        {
            if (contextId.isEmpty() || ids.isEmpty())
                return true;

            QVector<int> originalIds;
            originalIds.reserve(ids.size());
            for (const QString& text : ids)
            {
                const QString trimmed = text.trimmed();
                bool ok = false;
                const int id = trimmed.toInt(&ok);
                if (trimmed.isEmpty() || !ok || id < 0)
                {
                    errorMessage = QStringLiteral(
                        "几何 ID “%1” 不是有效的非负整数").arg(text);
                    return false;
                }
                originalIds.push_back(id);
            }

            QVector<int> currentIds;
            DirectModelingController* controller =
                DirectModelingController::getInstance();
            if (!controller
                || !controller->resolveScriptOriginalGeometryIds(
                    contextId, elementDim, originalIds, currentIds, &errorMessage))
            {
                if (errorMessage.isEmpty())
                    errorMessage = QStringLiteral("直接建模控制器不可用");
                return false;
            }

            ids.clear();
            for (int currentId : currentIds)
                ids.push_back(QString::number(currentId));
            return true;
        }

        // 约束、边界、载荷、接触等接口共用的点/面/体 ID 解析入口；
        // ti 为空指针表示该接口没有体参数（JiHeTi）。
        bool resolveScriptGeometryTargets(
            const APIServer::APIParams& params,
            QStringList& dian,
            QStringList& mian,
            QStringList* ti,
            QString& errorMessage)
        {
            const QString contextId = scriptOriginalGeometryIdContext(params);
            if (contextId.isEmpty())
                return true;

            struct TargetSpec { QStringList* ids; int dim; };
            TargetSpec specs[] = {
                { &dian, 0 },
                { &mian, 2 },
                { ti, 3 },
            };
            for (const TargetSpec& spec : specs)
            {
                if (!spec.ids)
                    continue;
                if (!resolveScriptGeometryIds(
                        contextId, spec.dim, *spec.ids, errorMessage))
                {
                    return false;
                }
            }
            return true;
        }
    }

    mPythonCommand* mPythonCommand::m_instance = nullptr;

    mPythonCommand::mPythonCommand(QObject* parent)
        : QObject(parent)
    {
    }

    mPythonCommand::~mPythonCommand()
    {
        unregisterAPI();
    }

    mPythonCommand* mPythonCommand::getInstance()
    {
        if (!m_instance)
            m_instance = new mPythonCommand;
        return m_instance;
    }

    void mPythonCommand::registerAPI()
    {
        registerPostExportAPIs();
        mGeoPythonAPIRegistrar::instance()->registerGeometryAPIs();
        mMeshPythonAPIRegistrar::instance()->registerMeshAPIs();
        mContactAPIRegistrar::instance()->registerContactAPIs();
        // 注册 API Server 回调：startSolver -> slot_startSolver
        getAPISchedulingServer->registerAPI("startSolver", this, &mPythonCommand::onStartSolverAPI, "启动求解");
        // 注册 API Server 回调：createFaceToFace -> onCreateFaceToFaceContact
        getAPISchedulingServer->registerAPI("createFaceToFaceContact", this, &mPythonCommand::onCreateFaceToFaceContactAPI, "创建面面接触");
        // 注册 API Server 回调：createSolverControl -> onCreateSolverControlAPI
        getAPISchedulingServer->registerAPI("createSolverControl", this, &mPythonCommand::onCreateSolverControlAPI, "创建求解控制");
        // 注册 API Server 回调：createFixedBoundCondit -> oncreateFixedBoundConditAPI
        getAPISchedulingServer->registerAPI("createFixedBoundCondit", this, &mPythonCommand::onCreateFixedBoundConditAPI, "创建固定约束边界");
        // 注册 API Server 回调：createFixedBoundCondit -> onCreateTemperatureBoundConditAPI
        getAPISchedulingServer->registerAPI("createTemperatureBoundCondit", this, &mPythonCommand::onCreateTemperatureBoundConditAPI, "创建温度边界");
        // 注册 API Server 回调：createLoadForce -> onCreateLoadForceAPI
        getAPISchedulingServer->registerAPI("createLoadForce", this, &mPythonCommand::onCreateLoadForceAPI, "创建集中力载荷");
        // 注册 API Server 回调：createLoadConvection -> onCreateLoadConvectionAPI
        getAPISchedulingServer->registerAPI("createLoadConvection", this, &mPythonCommand::onCreateLoadConvectionAPI, "创建对流换热载荷");
        // 注册 API Server 回调：createMaterial -> onCreateMaterialAPI
        getAPISchedulingServer->registerAPI("createIsotropicMaterial", this, &mPythonCommand::onCreateIsotropicMaterialAPI, "创建线弹性各向同性材料");
        // 注册 API Server 回调：createMaterial -> onCreateMaterialAPI
        getAPISchedulingServer->registerAPI("createHeatMaterial", this, &mPythonCommand::onCreateHeatMaterialAPI, "创建热学材料");
        // 注册 API Server 回调：createSolidProperty -> onCreateSolidPropertyAPI
        getAPISchedulingServer->registerAPI("createSolidProperty", this, &mPythonCommand::onCreateSolidPropertyAPI, "创建三维实体截面属性");
        // 注册 API Server 回调：createLinearStatic -> oncreateLinearStaticAPI
        getAPISchedulingServer->registerAPI("createLinearStatic", this, &mPythonCommand::onCreateLinearStaticAPI, "创建静力学分析");
        // 注册 API Server 回调：createSteadyHeat -> onCreateSteadyHeatAPI
        getAPISchedulingServer->registerAPI("createSteadyHeat", this, &mPythonCommand::onCreateSteadyHeatAPI, "创建稳态传热分析");
        // 注册 API Server 回调：createModalAnalysis -> onCreateModalAnalysisAPI
        getAPISchedulingServer->registerAPI("createModalAnalysis", this, &mPythonCommand::onCreateModalAnalysisAPI, "创建模态分析");
        // 注册 API Server 回调：createBind -> onCreateBindAPI
        getAPISchedulingServer->registerAPI("createBind", this, &mPythonCommand::onCreateBindAPI, "创建模态分析");
        // 注册 API Server 回调：createInitTempPredefinedField -> onCreateInitTempPredefinedFieldAPI
        getAPISchedulingServer->registerAPI("createInitTempPredefinedField", this, &mPythonCommand::onCreateInitTempPredefinedFieldAPI, "创建初始温度预定义场");
        // 注册 API Server 回调：createBind -> setViewDirection
        getAPISchedulingServer->registerAPI("setViewDirection", this, &mPythonCommand::onSetViewDirectionAPI, "设置视图方向");

        getAPISchedulingServer->registerAPI("optimTest", this, &mPythonCommand::onOptimTestAPI, "多学科优化测试");
    }

    void mPythonCommand::unregisterAPI()
    {
        unregisterPostExportAPIs();
        mGeoPythonAPIRegistrar::instance()->unregisterGeometryAPIs();
        mMeshPythonAPIRegistrar::instance()->unregisterMeshAPIs();
        mContactAPIRegistrar::instance()->unregisterContactAPIs();
        getAPISchedulingServer->unregisterAPI("startSolver");
        getAPISchedulingServer->unregisterAPI("createFaceToFaceContact");
        getAPISchedulingServer->unregisterAPI("createSolverControl"); 
        getAPISchedulingServer->unregisterAPI("createFixedBoundCondit");
        getAPISchedulingServer->unregisterAPI("createTemperatureBoundCondit");
        getAPISchedulingServer->unregisterAPI("createLoadForce");
        getAPISchedulingServer->unregisterAPI("createLoadConvection");
        getAPISchedulingServer->unregisterAPI("createIsotropicMaterial");
        getAPISchedulingServer->unregisterAPI("createHeatMaterial");
        getAPISchedulingServer->unregisterAPI("createSolidProperty");
        getAPISchedulingServer->unregisterAPI("createLinearStatic");
        getAPISchedulingServer->unregisterAPI("createSteadyHeat");
        getAPISchedulingServer->unregisterAPI("createModalAnalysis");
        getAPISchedulingServer->unregisterAPI("createBind");
        getAPISchedulingServer->unregisterAPI("createInitTempPredefinedField");
        getAPISchedulingServer->unregisterAPI("setViewDirection");

        getAPISchedulingServer->unregisterAPI("optimTest");
    }

    QVariant mPythonCommand::onOptimTestAPI(const APIServer::APIParams& params, QString& error)
    {
        if (getCVM->isPostModel())
        {
            return false;
        }
        QString MocData = R"({
    "algorithm": "PSO",
    "hyperparams": {
        "pop_size": 12,
        "max_iterations": 5,
        "seed": 42,
        "n_constraints": 0,
        "n_objectives": 1
    },
    "ranges": [[-10.0, 10.0], [-10.0, 10.0], [-10.0, 10.0]]
})";
        if (MxOptionTest::getInstance()->OptiomTest(MocData.toStdString()) != 0) {
            mGlobalSignals::getInstance()->outputMessageSig("多学科优化测试执行失败！");
            return false;
        }
        return true;
    }

    QVariant mPythonCommand::onStartSolverAPI(const APIServer::APIParams& params, QString& error)
    {
        if (getCVM->isPostModel())
        {
            return false;
        }
        vector<int> caseIDs = getCVM->getModelData()->getAllPhyID(Physical_Case);
        if (caseIDs.empty())
        {
            mGlobalSignals::getInstance()->outputMessageSig("无分析工况可求解！");
            return false;
        }
        mSolveTaskCenter::getInstance()->quickSolve(getCVM->getModelData()->getModelDataID(), caseIDs[0]);
        return true;
    }

    QVariant mPythonCommand::onCreateFaceToFaceContactAPI(const APIServer::APIParams& params, QString& error)
    {
        QStringList name = params.value("Name");
        QStringList zhumian = params.value("Master");
        QStringList congmian = params.value("Slave");
        QStringList xiaohuayi = params.value("XiaoHuaYi");
        QStringList yaliguoying = params.value("YaLiGuoYing");
        QStringList yueshufangfa = params.value("YueShuFangFa");
        QStringList suofangxishu = params.value("SuoFangXiShu");
        QStringList faxishu = params.value("FaXiShu");
        QStringList moca = params.value("MoCa");
        QStringList mocaxishu = params.value("MoCaXiShu");
        QStringList rejiechu = params.value("ReJieChu");
        QStringList tiaozhengzhi = params.value("TiaoZhengZhi");
        QStringList guoyingliang = params.value("GuoYingLiang");
        QStringList qiexiangzd = params.value("QieXiangZD");
        QStringList rejiechuzd = params.value("ReJieChuZD");
        QStringList jihezd = params.value("JiHeZD");
        QStringList jiechujianxizd = params.value("JieChuJianXiZD");

        //创建数据
        if (name.size() != 1)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "未识别到名称，面面接触创建失败！");
            return false;
        }
        if (getCVM->getModelData()->getManager<mContactManager>(Physical_Contact).ExistContactName(name[0]))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "名称已存在，面面接触创建失败！");
            return false;
        }
        int contactID = getCVM->getModelData()->getManager<mContactManager>(Physical_Contact).createID();
        MechContactData::mContactBase* _contactData = new mContactBase(contactID, SurfaceToSurfaceContact, name[0]);
        if (!_contactData)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "未知错误，面面接触创建失败！");
            return false;
        }
        getCVM->getModelData()->addPhysicalField<mContactBase>(_contactData);

        //创建主面内部集合
        if (zhumian.empty())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "未识别到主面，面面接触创建失败！");
            return false;
        }

        if (congmian.empty())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "未识别到从面，面面接触创建失败！");
            return false;
        }

        // original 模式：主/从面传入的是脚本最初导入时的面 ID，
        // 先通过直接建模的映射链换算成当前面 ID 再创建集合。
        {
            const QString contextId = scriptOriginalGeometryIdContext(params);
            QString resolveError;
            if (!contextId.isEmpty()
                && (!resolveScriptGeometryIds(contextId, 2, zhumian, resolveError)
                    || !resolveScriptGeometryIds(contextId, 2, congmian, resolveError)))
            {
                error = resolveError;
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
                    QStringLiteral("%1，面面接触创建失败！").arg(resolveError));
                return false;
            }
        }

        //获取映射
        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
        mMixSet* set = new mMixSet(setID, SetType::Mix, "contain", true);
        std::vector<RenderPickResultData> zhumianData;
        for (auto temp : zhumian)
        {
            RenderPickResultData tempdata;
            tempdata.result_id = temp.toInt();
            zhumianData.push_back(tempdata);
        }
        set->setMixSetData({ { MxFunctions::PickDataType::GeoFace,zhumianData } });
        getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set);
        mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_Contact, contactID }, { Physical_Set, setID }, MapRelation_Master);

        //创建从面内部集合
        setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
        set = new mMixSet(setID, SetType::Mix, "contain", true);
        std::vector<RenderPickResultData> congmianData;
        for (auto temp : congmian)
        {
            RenderPickResultData tempdata;
            tempdata.result_id = temp.toInt();
            congmianData.push_back(tempdata);
        }
        set->setMixSetData({ { MxFunctions::PickDataType::GeoFace,congmianData } });
        getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set);
        mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_Contact, contactID }, { Physical_Set, setID }, MapRelation_Slave);


        //接触参数折叠块逻辑
        if (xiaohuayi.size() == 1 && xiaohuayi[0] == "开启") //小滑移开启时，接触间隙折叠块显示 todo
        {
            _contactData->smallSlip = true;

            if (jiechujianxizd.size() == 1 && jiechujianxizd[0].toInt() == 1)
            {
                _contactData->gap_define = true;
                if (guoyingliang.size() == 1)_contactData->gap_overclosure = guoyingliang[0].toDouble();
            }
        }

        //法向接触行为折叠块逻辑
        if (yaliguoying.size() == 1)
        {
            _contactData->overclosure = yaliguoying[0];
            if (yaliguoying[0] == "线性接触") //当压力过盈为线性接触时只需读取罚系数
            {
                if (faxishu.size() == 1)
                {
                    _contactData->penalty_factor = faxishu[0];
                }
            }
            else //当压力过盈不为线性接触时需读约束方法
            {
                if (yueshufangfa.size() == 1)
                {
                    _contactData->constraint = yueshufangfa[0];
                    if (yueshufangfa[0] == "罚函数法")//当约束方法为罚函数法时需要读缩放系数
                    {
                        if (suofangxishu.size() == 1) _contactData->scale_factor = suofangxishu[0];
                    }
                }
            }
        }

        //切向行为折叠块逻辑
        if (qiexiangzd.size() == 1 && qiexiangzd[0].toInt() == 1) //切向行为折叠块设为1是表示勾选  0表示不勾选
        {
            _contactData->tangential_define = true;
            if (moca.size() == 1)            //读取摩擦公式
            {
                _contactData->tangential_friction = moca[0];
                if (moca[0] == "通用接触摩擦") //读取静摩擦系数
                {
                    if (mocaxishu.size() == 1) _contactData->tangential_static_coef = mocaxishu[0].toFloat();
                }
            }
        }

        //热接触间隙传导折叠块逻辑
        if (rejiechuzd.size() == 1 && qiexiangzd[0].toInt() == 1)
        {
            _contactData->heat_define = true;
            //读取表格数据
            if (rejiechu.size() >= 0)
            {
                _contactData->heat_value.clear();
                for (int i = 0; i < rejiechu.size(); i += 2)
                {
                    double first = rejiechu[i].toDouble();
                    double second = (i + 1 < rejiechu.size()) ? rejiechu[i + 1].toDouble() : 0.0;
                    _contactData->heat_value.append({ first, second });
                }
            }
        }

        //几何调整折叠框
        if (jihezd.size() == 1 && qiexiangzd[0].toInt() == 1)
        {
            _contactData->geom_define = true;
            if (tiaozhengzhi.size() == 1) _contactData->geom_value = tiaozhengzhi[0].toDouble();
        }

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateContactSig(true, _contactData->getContactId());
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建面面接触 %1").arg(_contactData->getContactName()), true);
        return true;
    }

    QVariant mPythonCommand::onCreateSolverControlAPI(const APIServer::APIParams& params, QString& error)
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        if (globalParam->getSolveControlData())
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("求解控制已存在，只能创建一个求解控制"));
            return false;
        }

        globalParam->createSolveControlData();

        QStringList cpuCores = params.value("CPUCores");
        if (cpuCores.size() == 1)
        {
            globalParam->getSolveControlData()->setCpuCoreCount(cpuCores[0]);
        }

        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建求解控制"));
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSolveControlSig(true);

        return true;
    }

    QVariant mPythonCommand::onCreateInitTempPredefinedFieldAPI(const APIServer::APIParams& params, QString& error)
    {
        QStringList name = params.value("Name");
        QStringList dian = params.value("JiHeDian");
        QStringList mian = params.value("JiHeMian");
        QStringList ti = params.value("JiHeTi");
        QStringList defineType = params.value("defineType");
        QStringList temp = params.value("Temp");

        if (name.size() != 1)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "未识别到名称，初始温度预定义场创建失败！");
            return false;
        }

        if (getCVM->getModelData()->getManager<mPredefinedFieldManager>(Physical_PredefinedField).ExistPredefinedFieldName(name[0]))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "名称已存在，初始温度预定义场创建失败！");
            return false;
        }

        if (dian.empty() && mian.empty()&& ti.empty())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "作用对象无效，初始温度预定义场创建失败！");
            return false;
        }

        // original 模式：把最初导入时的几何 ID 换算成直接建模后的当前 ID。
        {
            QString resolveError;
            if (!resolveScriptGeometryTargets(params, dian, mian, &ti, resolveError))
            {
                error = resolveError;
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
                    QStringLiteral("%1，初始温度预定义场创建失败！").arg(resolveError));
                return false;
            }
        }

        //数据
        int id = getCVM->getModelData()->getManager<mPredefinedFieldManager>(Physical_PredefinedField).createID();
        auto newpredefinefield = std::make_shared<MechCaseData::mPredefinedFieldInitTemp>(id, PREDEFINED_FIELD_INIT_TEMP, name[0]);
        getCVM->getModelData()->addPhysicalFieldP(newpredefinefield);

        //获取映射
        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
        mMixSet* set = new mMixSet(setID, SetType::Mix, "contain", true);
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> Mixdata;

        std::vector<RenderPickResultData> dianData;
        for (auto temp : dian)
        {
            RenderPickResultData tempdata;
            tempdata.result_id = temp.toInt();
            dianData.push_back(tempdata);
        }
        if (!dianData.empty()) {
            Mixdata[MxFunctions::PickDataType::GeoPoint] = dianData;
        }

        std::vector<RenderPickResultData> mianData;
        for (auto temp : mian)
        {
            RenderPickResultData tempdata;
            tempdata.result_id = temp.toInt();
            mianData.push_back(tempdata);
        }
        if (!mianData.empty()) {
            Mixdata[MxFunctions::PickDataType::GeoFace] = mianData;
        }
        std::vector<RenderPickResultData> tiData;
        for (auto temp : ti)
        {
            RenderPickResultData tempdata;
            tempdata.result_id = temp.toInt();
            tiData.push_back(tempdata);
        }
        if (!tiData.empty()) {
            Mixdata[MxFunctions::PickDataType::GeoSolid] = tiData;
        }

        set->setMixSetData(Mixdata);
        getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set);
        mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_PredefinedField, newpredefinefield->getPredefinedFieldId() }, { Physical_Set, setID });

        if (defineType.size() == 1)
        {
            newpredefinefield->_defineType = defineType[0].toInt()%2;
        }

        if (temp.size() == 1) {
            newpredefinefield->_temp = temp[0];
        }

        //创建完成
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建预定义场 %1").arg(newpredefinefield->getPredefinedFieldName()), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePredefinedFieldSig(true, newpredefinefield->getPredefinedFieldId());

        return true;
    }

    QVariant mPythonCommand::onSetViewDirectionAPI(const APIServer::APIParams& params, QString& error) {
        QStringList mode = params.value("mode");
        if (mode.size() != 1)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "未识别到方向参数，视角方向切换失败！");
            return false;
        }
        if (!getCVM)
        {
            return false;
        }
        mBaseRend* rend = nullptr;

        auto viewportPrivate = VMManager->getCurrentViewportPrivate();
        if (viewportPrivate == nullptr)
        {
            return false;
        }
        rend = viewportPrivate->getRendWidget();
        if (rend == nullptr)
        {
            return false;
        }

        switch (mode[0].toInt())
        {
        case 0:rend->SetPerspective(Perspective::Front);
            break;
        case 1:rend->SetPerspective(Perspective::Rear);
            break;
        case 2:rend->SetPerspective(Perspective::Top);
            break;
        case 3:rend->SetPerspective(Perspective::Bottom);
            break;
        case 4:rend->SetPerspective(Perspective::Left);
            break;
        case 5:rend->SetPerspective(Perspective::Right);
            break;
        case 6:rend->SetPerspective(Perspective::ISO1);
            break;
        default:
            break;
        }

        VMManager->setCameraSyncTimer(500, 30, [rend]() {
            emit rend->signal_CameraSync();
            });
        rend->updatePaint();
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功切换视图方向"), true);
        return true;
    }
  

    QVariant mPythonCommand::onCreateTemperatureBoundConditAPI(const APIServer::APIParams& params, QString& error)
    {
        QStringList name = params.value("Name");
        QStringList dian = params.value("JiHeDian");
        QStringList mian = params.value("JiHeMian");
        QStringList temp = params.value("Temp");
        QStringList hanshu = params.value("HanShu");

        if (name.size() != 1)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "未识别到名称，温度边界创建失败！");
            return false;
        }

        if (getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).ExistLoadBoundaryName(name[0]))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "名称已存在，温度边界创建失败！");
            return false;
        }

        if (dian.empty() && mian.empty())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "作用对象无效，温度边界创建失败！");
            return false;
        }

        // original 模式：把最初导入时的几何 ID 换算成直接建模后的当前 ID。
        {
            QString resolveError;
            if (!resolveScriptGeometryTargets(params, dian, mian, nullptr, resolveError))
            {
                error = resolveError;
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
                    QStringLiteral("%1，温度边界创建失败！").arg(resolveError));
                return false;
            }
        }

        //数据
        int id = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        auto newboundary = std::make_shared<MechCaseData::mBoundaryTemp>(id, BOUNDARY_IM_TEMP, name[0]);
        getCVM->getModelData()->addPhysicalFieldP(newboundary, -1);

        //获取映射
        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
        mMixSet* set = new mMixSet(setID, SetType::Mix, "contain", true);
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> Mixdata;

        std::vector<RenderPickResultData> dianData;
        for (auto temp : dian)
        {
            RenderPickResultData tempdata;
            tempdata.result_id = temp.toInt();
            dianData.push_back(tempdata);
        }
        if (!dianData.empty()) {
            Mixdata[MxFunctions::PickDataType::GeoPoint] = dianData;
        }

        std::vector<RenderPickResultData> mianData;
        for (auto temp : mian)
        {
            RenderPickResultData tempdata;
            tempdata.result_id = temp.toInt();
            mianData.push_back(tempdata);
        }
        if (!mianData.empty()) {
            Mixdata[MxFunctions::PickDataType::GeoFace] = mianData;
        }

        set->setMixSetData(Mixdata);

        getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set);
        mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_LoadBoundary, newboundary->getLoadBoundaryId() }, { Physical_Set, setID });

        if (temp.size() == 1)
        {
            newboundary->_temp = temp[0].toDouble();
        }
      
        if (hanshu.size() == 1) {
            int id = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getIDByFunctionName(hanshu[0]);
            if (id != 1) {
                mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_LoadBoundary, newboundary->getLoadBoundaryId() }, { Physical_Function, id });
            }
        }

        //创建完成
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建边界 %1").arg(newboundary->getLoadBoundaryName()), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateBoundarySig(true, newboundary->getLoadBoundaryId());

        return true;
    }

    QVariant mPythonCommand::onCreateFixedBoundConditAPI(const APIServer::APIParams& params, QString& error)
    {
        QStringList name = params.value("Name");
        QStringList dian = params.value("JiHeDian");
        QStringList mian = params.value("JiHeMian");
        QStringList type = params.value("FixedType");

        if (name.size() != 1)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "未识别到名称，固定约束创建失败！");
            return false;
        }

        if (getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).ExistLoadBoundaryName(name[0]))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "名称已存在，固定约束创建失败！");
            return false;
        }

        if (dian.empty() && mian.empty())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "作用对象无效，固定约束创建失败！");
            return false;
        }

        // original 模式：把最初导入时的几何 ID 换算成直接建模后的当前 ID。
        {
            QString resolveError;
            if (!resolveScriptGeometryTargets(params, dian, mian, nullptr, resolveError))
            {
                error = resolveError;
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
                    QStringLiteral("%1，固定约束创建失败！").arg(resolveError));
                return false;
            }
        }

        //数据
        int id = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        auto newboundary = std::make_shared<MechCaseData::mBoundaryFixed>(id, BOUNDARY_IM_FIXED, name[0]);
        getCVM->getModelData()->addPhysicalFieldP(newboundary, -1);

        //获取映射
        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
        mMixSet* set = new mMixSet(setID, SetType::Mix, "contain", true);
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> Mixdata;

        std::vector<RenderPickResultData> dianData;
        for (auto temp : dian)
        {
            RenderPickResultData tempdata;
            tempdata.result_id = temp.toInt();
            dianData.push_back(tempdata);
        }
        if (!dianData.empty()) {
            Mixdata[MxFunctions::PickDataType::GeoPoint] = dianData;
        }

        std::vector<RenderPickResultData> mianData;
        for (auto temp : mian)
        {
            RenderPickResultData tempdata;
            tempdata.result_id = temp.toInt();
            mianData.push_back(tempdata);
        }
        if (!mianData.empty()) {
            Mixdata[MxFunctions::PickDataType::GeoFace] = mianData;
        }

        set->setMixSetData(Mixdata);

        getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set);
        mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_LoadBoundary, newboundary->getLoadBoundaryId() }, { Physical_Set, setID });

        if (type.size() == 1)
        {
            if (type[0] == "完全固定")
            {
                newboundary->_fixedType = 0;
            }
            else if (type[0] == "铰链")
            {
                newboundary->_fixedType = 1;
            }
            else if (type[0] == "关于X轴对称")
            {
                newboundary->_fixedType = 2;
            }
            else if (type[0] == "关于Y轴对称")
            {
                newboundary->_fixedType = 3;
            }
            else if (type[0] == "关于Z轴对称")
            {
                newboundary->_fixedType = 4;
            }
            else if (type[0] == "关于X轴反对称")
            {
                newboundary->_fixedType = 5;
            }
            else if (type[0] == "关于Y轴反对称")
            {
                newboundary->_fixedType = 6;
            }
            else if (type[0] == "关于Z轴反对称")
            {
                newboundary->_fixedType = 7;
            }
        }

        //创建完成
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建边界 %1").arg(newboundary->getLoadBoundaryName()), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateBoundarySig(true, newboundary->getLoadBoundaryId());

        return true;
    }

    QVariant mPythonCommand::onCreateLoadConvectionAPI(const APIServer::APIParams& params, QString& error)
    {
        QStringList name = params.value("Name");

        QStringList dian = params.value("JiHeDian");
        QStringList mian = params.value("JiHeMian");

        QStringList defineType = params.value("defineType");
        QStringList Template = params.value("Template");
        QStringList HeatValue = params.value("HeatValue");
        QStringList TemplateHanshu = params.value("TemplateHanshu");
        QStringList HeatHanshu = params.value("HeatHanshu");

        if (name.size() != 1)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "未识别到名称，对流换热载荷创建失败！");
            return false;
        }

        if (getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).ExistLoadBoundaryName(name[0]))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "名称已存在，对流换热载荷创建失败！");
            return false;
        }

        if (dian.empty() && mian.empty())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "作用对象无效，对流换热载荷创建失败！");
            return false;
        }

        // original 模式：把最初导入时的几何 ID 换算成直接建模后的当前 ID。
        {
            QString resolveError;
            if (!resolveScriptGeometryTargets(params, dian, mian, nullptr, resolveError))
            {
                error = resolveError;
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
                    QStringLiteral("%1，对流换热载荷创建失败！").arg(resolveError));
                return false;
            }
        }

        //数据
        int id = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        auto newload = std::make_shared<MechCaseData::mLoadConvection>(id, LOAD_IM_CONVECTION, name[0]);
        getCVM->getModelData()->addPhysicalFieldP(newload);

        //获取映射
        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
        mMixSet* set = new mMixSet(setID, SetType::Mix, "contain", true);

        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> Mixdata;

        std::vector<RenderPickResultData> dianData;
        for (auto temp : dian)
        {
            RenderPickResultData tempdata;
            tempdata.result_id = temp.toInt();
            dianData.push_back(tempdata);
        }
        if (!dianData.empty()) {
            Mixdata[MxFunctions::PickDataType::GeoPoint] = dianData;
        }

        std::vector<RenderPickResultData> mianData;
        for (auto temp : mian)
        {
            RenderPickResultData tempdata;
            tempdata.result_id = temp.toInt();
            mianData.push_back(tempdata);
        }
        if (!mianData.empty()) {
            Mixdata[MxFunctions::PickDataType::GeoFace] = mianData;
        }

        set->setMixSetData(Mixdata);

        getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set);
        mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_LoadBoundary, newload->getLoadBoundaryId() }, { Physical_Set, setID });

        if (defineType.size() == 1) newload->defineType_ = defineType[0].toInt();
        if (Template.size() == 1) newload->V1 = Template[0];
        if (HeatValue.size() == 1) newload->V2 = HeatValue[0];

        if (TemplateHanshu.size() == 1)
        {
            int id = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getIDByFunctionName(TemplateHanshu[0]);
            if (id != -1) //函数不存在则提示
            {
                mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_LoadBoundary, newload->getLoadBoundaryId() }, { Physical_Function, id });
            }
        }
        if (HeatHanshu.size() == 1)
        {
            int id = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getIDByFunctionName(HeatHanshu[0]);
            if (id != -1) //函数不存在则提示
            {
                mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_LoadBoundary, newload->getLoadBoundaryId() }, { Physical_Function, id });
            }
        }

        //创建完成
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建载荷 %1").arg(newload->getLoadBoundaryName()), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateLoadSig(true, newload->getLoadBoundaryId());
        return true;
    }


    QVariant mPythonCommand::onCreateLoadForceAPI(const APIServer::APIParams& params, QString& error)
    {
        QStringList name = params.value("Name");
        QStringList dian = params.value("JiHeDian");
        QStringList mian = params.value("JiHeMian");
        QStringList x = params.value("X");
        QStringList y = params.value("Y");
        QStringList z = params.value("Z");
        QStringList hanshu = params.value("HanShu");
        QStringList zuobiaoxi = params.value("ZuoBiaoXi");

        if (name.size() != 1)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "未识别到名称，集中力创建失败！");
            return false;
        }

        if (getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).ExistLoadBoundaryName(name[0]))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "名称已存在，集中力创建失败！");
            return false;
        }

        if (dian.empty() && mian.empty())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "作用对象无效，集中力创建失败！");
            return false;
        }

        // original 模式：把最初导入时的几何 ID 换算成直接建模后的当前 ID。
        {
            QString resolveError;
            if (!resolveScriptGeometryTargets(params, dian, mian, nullptr, resolveError))
            {
                error = resolveError;
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
                    QStringLiteral("%1，集中力创建失败！").arg(resolveError));
                return false;
            }
        }

        //数据
        int id = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        auto newload = std::make_shared<MechCaseData::mLoadForce>(id, LOAD_IM_FORCE, name[0]);
        getCVM->getModelData()->addPhysicalFieldP(newload);

        //获取映射
        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
        mMixSet* set = new mMixSet(setID, SetType::Mix, "contain", true);
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> Mixdata;

        std::vector<RenderPickResultData> dianData;
        for (auto temp : dian)
        {
            RenderPickResultData tempdata;
            tempdata.result_id = temp.toInt();
            dianData.push_back(tempdata);
        }
        if (!dianData.empty()) {
            Mixdata[MxFunctions::PickDataType::GeoPoint] = dianData;
        }

        std::vector<RenderPickResultData> mianData;
        for (auto temp : mian)
        {
            RenderPickResultData tempdata;
            tempdata.result_id = temp.toInt();
            mianData.push_back(tempdata);
        }
        if (!mianData.empty()) {
            Mixdata[MxFunctions::PickDataType::GeoFace] = mianData;
        }

        set->setMixSetData(Mixdata);

        getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set);
        mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_LoadBoundary, newload->getLoadBoundaryId() }, { Physical_Set, setID });

        if (x.size() == 1) newload->force_x = x[0];
        if (y.size() == 1) newload->force_y = y[0];
        if (z.size() == 1) newload->force_z = z[0];

        if (hanshu.size() == 1)
        {
            int id = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getIDByFunctionName(hanshu[0]);
            if (id != -1) //函数不存在则提示
            {
                mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_LoadBoundary, newload->getLoadBoundaryId() }, { Physical_Function, id });
            }
        }

        if (zuobiaoxi.size() == 1)
        {
            int id = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getIDByCoordinateName(zuobiaoxi[0]);
            if (id != -1)
            {
                mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_LoadBoundary, newload->getLoadBoundaryId() }, { Physical_Coordinate, id });
            }
        }

        //创建完成
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建载荷 %1").arg(newload->getLoadBoundaryName()), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateLoadSig(true, newload->getLoadBoundaryId());
        return true;
    }

    QVariant mPythonCommand::onCreateHeatMaterialAPI(const APIServer::APIParams& params, QString& error) {
        QStringList matName = params.value("matName");  //材料名
        QStringList Val1 = params.value("Val1");        //热传导系数
        QStringList density = params.value("density");  //密度
        QStringList specificHeat = params.value("specificHeat");    //比热容系数
        QStringList generationRate = params.value("generationRate");    //热生成率
        if (matName.size() != 1 || Val1.size() != 1 || density.size() != 1 || specificHeat.size() != 1 || generationRate.size() != 1)
        {
            return false;
        }
        if (getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).ExistMaterialName(matName[0])) {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "名称已存在，材料创建失败！");
            return false;
        }
        int matID = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).createID();
        int ColorSize = PartColorLoop.size();
        QVector4D vec4d = PartColorLoop[(matID - 1) % ColorSize];
        QColor color = QColor(vec4d.x() * 255, vec4d.y() * 255, vec4d.z() * 255, vec4d.w() * 255);
        shared_ptr<MechMaterialData::mMaterialBase> mat = make_shared<MechMaterialData::mMaterialBase>(matID, color, Thermal_Base, matName[0]);
        QVariant variant;
        variant = Val1[0];
        mat->addProperty(MAT_HeatConductivityCoefficient_UnCheck, MatProp_HeatConductivityCoefficient_A1, variant);
        variant = density[0];
        mat->addProperty(MAT_Density, MatProp_Density, variant);
        variant = specificHeat[0];
        mat->addProperty(MAT_Heat, MatProp_HeatCapacityDensity, variant);
        variant = generationRate[0];
        mat->addProperty(MAT_HeatGenerationRate, MatProp_HeatGenerationRate, variant);
        getCVM->getModelData()->addPhysicalFieldP(mat);
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建材料 %1").arg(matName[0]), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMaterialSig(true, mat->getMaterialId());
        return true;
    }

    QVariant mPythonCommand::onCreateIsotropicMaterialAPI(const APIServer::APIParams& params, QString& error)
    {
        QStringList matName = params.value("matName");
        QStringList youngsModulus = params.value("youngsModulus");
        QStringList botson = params.value("botson");
        QStringList density = params.value("density");
        if (matName.size() != 1 || youngsModulus.size() != 1 || botson.size() != 1 || density.size() != 1)
        {
            return false;
        }
        if (getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).ExistMaterialName(matName[0]))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "名称已存在，材料创建失败！");
            return false;
        }
        int matID = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).createID();
        int nColor = PartColorLoop.size();
        QVector4D vector4d = PartColorLoop[(matID - 1) % nColor];
        QColor color = QColor(vector4d.x() * 255, vector4d.y() * 255, vector4d.z() * 255, vector4d.w() * 255);
        shared_ptr<MechMaterialData::mMaterialBase> mat = make_shared<MechMaterialData::mMaterialBase>(matID, color, LinearElastic_Isotropic, matName[0]);
        QVariant variant;
        variant = youngsModulus[0];
        mat->addProperty(MAT_Isotropy, MatProp_YoungsModulus, variant);
        variant = botson[0].toFloat();
        mat->addProperty(MAT_Isotropy, MatProp_Botson, variant);
        variant = density[0].toFloat();
        mat->addProperty(MAT_Density, MatProp_Density, variant);

        getCVM->getModelData()->addPhysicalFieldP(mat);
        //创建完成
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建材料 %1").arg(matName[0]), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMaterialSig(true, mat->getMaterialId());
        return true;
    }

    QVariant mPythonCommand::onCreateSolidPropertyAPI(const APIServer::APIParams& params, QString& error)
    {
        QStringList name = params.value("name");
        QStringList matName = params.value("matName");
        QStringList partNames = params.value("partName");
        QStringList coordinateName = params.value("coordinateName");
        if (name.size() != 1 || matName.size() != 1 || coordinateName.size() != 1)
        {
            return false;
        }
        if (getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).ExistPropertyName(name[0]))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "名称已存在，三维截面实体创建失败！");
            return false;
        }
        int ID = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
        MechPropertyData::mSolidHomo* property = new MechPropertyData::mSolidHomo(ID, SolidProperty, name[0]);
        getCVM->getModelData()->addPhysicalField<MechPropertyData::mPropertyBase>(property);
        //获取映射
        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        int matId = getCVM->getModelData()->getPhyIDByName(Physical_Material, matName[0]);
        mapRelationManager.appendMapRelation({ Physical_Property, ID }, { Physical_Material, matId }, MapRelation_None);
        int coorId = getCVM->getModelData()->getPhyIDByName(Physical_Coordinate, coordinateName[0]);
        mapRelationManager.appendMapRelation({ Physical_Property, ID }, { Physical_Coordinate, coorId }, MapRelation_None);
        for (auto partName : partNames)
        {
            int partId = getCVM->getModelData()->getPhyIDByName(Physical_Part, partName);
            mapRelationManager.appendMapRelation({ Physical_Part, partId }, { Physical_Property, ID }, MapRelation_None);
        }
        //创建完成
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建三维截面实体 %1").arg(name[0]), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePropertySig(true, ID);
        return true;
    }

    QVariant mPythonCommand::onCreateSteadyHeatAPI(const APIServer::APIParams& params, QString& error)
    {
        QStringList name = params.value("name");
        QStringList timeStepType = params.value("timeStepType");
        QStringList initialStepValue = params.value("initialStepValue");
        QStringList totolTimeValue = params.value("totolTimeValue");
        QStringList minTimeStep = params.value("minTimeStep");
        QStringList maxTimeStep = params.value("maxTimeStep");
        QStringList cntnlsub = params.value("cntnlsub");
        QStringList outputStr = params.value("outputStr");
        QStringList loadStr = params.value("loadStr");
        QStringList boundaryStr = params.value("boundaryStr");
        QStringList preDefineFieldStr = params.value("preDefineFieldStr");
        if (name.size() != 1  )
        {
            return false;
        }
        if (getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).ExistCaseName(name[0]))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "名称已存在，稳态传热分析创建失败！");
            return false;
        }
        int caseID = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).createID();
        MechCaseData::mCaseImplicitSteadyHeat* caseData = new MechCaseData::mCaseImplicitSteadyHeat(caseID, ImplicitSteadyHeat, name[0]);
        getCVM->getModelData()->addPhysicalField<MechCaseData::mCaseBase>(caseData);
        MechCaseData::mCaseBase* caseBase = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).getCaseData(caseID);
        if (!timeStepType.isEmpty())
        {
            if (!timeStepType[0].compare("auto", Qt::CaseInsensitive))
            {
                caseData->_timeStepType = "自动";
            }
            else if (!timeStepType[0].compare("fix", Qt::CaseInsensitive))
            {
                caseData->_timeStepType = "固定";
            }
        }
        if (!cntnlsub.isEmpty()) {
            if (!cntnlsub[0].compare("on", Qt::CaseInsensitive))
            {
                caseData->_cntnlsub = "开启";
            }
            else if (!cntnlsub[0].compare("off", Qt::CaseInsensitive))
            {
                caseData->_cntnlsub = "关闭";
            }
        }
        if (!initialStepValue.isEmpty()) {
            caseData->_initialStepValue = initialStepValue[0];
        }
        if (!totolTimeValue.isEmpty()) {
            caseData->_totolTimeValue = totolTimeValue[0];
        }
        if (!minTimeStep.isEmpty()) {
            caseData->_minTimeStep = minTimeStep[0];
        }
        if (!maxTimeStep.isEmpty()) {
            caseData->_maxTimeStep = maxTimeStep[0];
        }
        
        pair<QString, QString> key;
        QHash<QString, QStringList> outputs;
        for (auto temp : outputStr)
        {
            key = mVariableFunction::getOutPutPairByVariable(temp);
            outputs[key.first].append(key.second);
        }
        if (!outputs.isEmpty())
            caseBase->setOutputs(outputs);
        
        //创建完成
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建稳态传热分析 %1").arg(name[0]), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCaseSig(true, caseID);
        
        MechMapRelation::mMapRelationManager& manager = getCVM->getModelData()->getMapRelationManager();
        for (auto temp : loadStr)
        {
            int phyID = getCVM->getModelData()->getPhyIDByName(Physical_LoadBoundary, temp);
            manager.appendMapRelation({ Physical_LoadBoundary, phyID }, { Physical_Case, caseID });
            if (phyID != -1)
            {
                emit MechCommon::mMechGlobalSignals::getInstance()->updataLoadBoundaryTreeView(caseID, phyID, 0);
            }
        }
        for (auto temp : boundaryStr)
        {
            int phyID = getCVM->getModelData()->getPhyIDByName(Physical_LoadBoundary, temp);
            manager.appendMapRelation({ Physical_LoadBoundary, phyID }, { Physical_Case, caseID });
            if (phyID != -1)
            {
                emit MechCommon::mMechGlobalSignals::getInstance()->updataLoadBoundaryTreeView(caseID, phyID, 0);
            }
        }
        for (auto temp : preDefineFieldStr) {
            int phyID = getCVM->getModelData()->getPhyIDByName(Physical_PredefinedField, temp);
            manager.appendMapRelation({ Physical_PredefinedField, phyID }, { Physical_Case, caseID });
            if (phyID != -1) {
                emit MechCommon::mMechGlobalSignals::getInstance()->updatePredefinedFieldTreeView(caseID, phyID, 0);
            }
        }
        
        return true;
    }

    QVariant mPythonCommand::onCreateLinearStaticAPI(const APIServer::APIParams& params, QString& error)
    {
        QStringList name = params.value("name");
        QStringList solver = params.value("solver");
        QStringList inrel = params.value("inrel");
        QStringList autospc = params.value("autospc");
        QStringList outputStr = params.value("output");
        QStringList loadStr = params.value("load");
        QStringList boundaryStr = params.value("boundary");
        if (name.size() != 1 || solver.size() != 1 || inrel.size() != 1)
        {
            return false;
        }
        if (getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).ExistCaseName(name[0]))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "名称已存在，线性静态分析创建失败！");
            return false;
        }
        int caseID = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).createID();
        MechCaseData::mCaseImplicitLinearStatic* caseData = new MechCaseData::mCaseImplicitLinearStatic(caseID, ImplicitLinearStatic, name[0]);
        getCVM->getModelData()->addPhysicalField<MechCaseData::mCaseBase>(caseData);
        MechCaseData::mCaseBase* caseBase = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).getCaseData(caseID);
        if (!inrel.isEmpty())
        {
            if (!inrel[0].compare("0", Qt::CaseInsensitive))
            {
                caseData->_inertiaRelief = "关闭";
            }
            else if (!inrel[0].compare("-1", Qt::CaseInsensitive))
            {
                caseData->_inertiaRelief = "手动开启";
            }
            else if (!inrel[0].compare("-2", Qt::CaseInsensitive))
            {
                caseData->_inertiaRelief = "自动开启";
            }
        }
        if (!autospc.isEmpty()) {
            if (!autospc[0].compare("on", Qt::CaseInsensitive))
            {
                caseData->_autospc = "开启";
            }
            else if (!autospc[0].compare("off", Qt::CaseInsensitive))
            {
                caseData->_autospc = "关闭";
            }
        }
        if (!solver.isEmpty())
        {
            if (!solver[0].compare("dsv", Qt::CaseInsensitive))
            {
                caseData->_equationSolver = "直接法求解器DSV";
            }
            else if (!solver[0].compare("pariters", Qt::CaseInsensitive))
            {
                caseData->_equationSolver = "迭代法求解器Pariters";
            }
        }
        pair<QString, QString> key;
        QHash<QString, QStringList> outputs;
        for (auto temp : outputStr)
        {
            key = mVariableFunction::getOutPutPairByVariable(temp);
            outputs[key.first].append(key.second);
        }
        if (!outputs.isEmpty())
            caseBase->setOutputs(outputs);

        //创建完成
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建线性静态分析 %1").arg(name[0]), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCaseSig(true, caseID);

        MechMapRelation::mMapRelationManager& manager = getCVM->getModelData()->getMapRelationManager();
        for (auto temp : loadStr)
        {
            int phyID = getCVM->getModelData()->getPhyIDByName(Physical_LoadBoundary, temp);
            manager.appendMapRelation({ Physical_LoadBoundary, phyID }, { Physical_Case, caseID });
            if (phyID != -1)
            {
                emit MechCommon::mMechGlobalSignals::getInstance()->updataLoadBoundaryTreeView(caseID, phyID, 0);
            }
        }
        for (auto temp : boundaryStr)
        {
            int phyID = getCVM->getModelData()->getPhyIDByName(Physical_LoadBoundary, temp);
            manager.appendMapRelation({ Physical_LoadBoundary, phyID }, { Physical_Case, caseID });
            if (phyID != -1)
            {
                emit MechCommon::mMechGlobalSignals::getInstance()->updataLoadBoundaryTreeView(caseID, phyID, 0);
            }
        }

        return true;
    }

    QVariant mPythonCommand::onCreateModalAnalysisAPI(const APIServer::APIParams& params, QString& error)
    {
        QStringList name = params.value("Name");
        QStringList qiujie = params.value("QiuJie");
        QStringList dabianxing = params.value("DaBianXing");
        QStringList zuixiao = params.value("MinPinLv");
        QStringList zuida = params.value("MaxPinLv");
        QStringList jieshu = params.value("JieShu");
        QStringList rongcha = params.value("RongCha");
        QStringList xishu = params.value("XiShu");
        QStringList tezheng = params.value("TeZheng");
        QStringList shuchu = params.value("ShuChu");

        if (name.size() != 1)
        {
            return false;
        }

        if (getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).ExistCaseName(name[0]))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "名称已存在，模态分析创建失败！");
            return false;
        }

        int caseID = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).createID();
        MechCaseData::mCaseImplicitModal* caseData = new MechCaseData::mCaseImplicitModal(caseID, ImplicitModal, name[0]);
        getCVM->getModelData()->addPhysicalField<MechCaseData::mCaseBase>(caseData);

        if (qiujie.size() == 1) caseData->_modalSolver = qiujie[0];
        if (dabianxing.size() == 1 && dabianxing[0].toInt() == 1) caseData->_nlgeo = true;
        if (zuixiao.size() == 1) caseData->_minFrequency = zuixiao[0];
        if (zuida.size() == 1) caseData->_maxFrequency = zuida[0];
        if (jieshu.size() == 1) caseData->_order = jieshu[0];
        if (rongcha.size() == 1) caseData->_tolerance = rongcha[0];
        if (tezheng.size() == 1) caseData->_normalization = tezheng[0];
        caseData->_modalSolver = "自动多重子结构";
        std::pair<QString, QString> key;
        QHash<QString, QStringList> outputs;
        for (auto temp : shuchu)
        {
            key = mVariableFunction::getOutPutPairByVariable(temp);
            outputs[key.first].append(key.second);
        }
        if(!outputs.isEmpty())
            caseData->setOutputs(outputs);

        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建模态分析 %1").arg(name[0]), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCaseSig(true, caseID);

        return true;
    }

    QVariant mPythonCommand::onCreateBindAPI(const APIServer::APIParams& params, QString& error)
    {
        QStringList name = params.value("Name");
        QStringList zhumian = params.value("ZhuMian");
        QStringList congmian = params.value("CongMian");
        QStringList leixing = params.value("LeiXing");
        QStringList rongchaleixing = params.value("RongChaLeiXing");
        QStringList rongcha = params.value("RongCha");
        QStringList tiaozheng = params.value("TiaoZheng");

        if (name.size() != 1 || zhumian.isEmpty() || congmian.isEmpty())
        {
            return false;
        }

        // original 模式：主/从面传入的是脚本最初导入时的面 ID，
        // 先通过直接建模的映射链换算成当前面 ID 再创建集合。
        {
            const QString contextId = scriptOriginalGeometryIdContext(params);
            QString resolveError;
            if (!contextId.isEmpty()
                && (!resolveScriptGeometryIds(contextId, 2, zhumian, resolveError)
                    || !resolveScriptGeometryIds(contextId, 2, congmian, resolveError)))
            {
                error = resolveError;
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
                    QStringLiteral("%1，绑定创建失败！").arg(resolveError));
                return false;
            }
        }

        if (getCVM->getModelData()->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).ExistConnectorName(name[0]))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, "名称已存在，绑定创建失败！");
            return false;
        }

        int id = getCVM->getModelData()->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).createID();
        auto Data = std::make_shared<MechConnectorData::mConnectorBind>(id, Connector_Bind, name[0]);
        getCVM->getModelData()->addPhysicalFieldP(Data);

        //获取映射
        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
        mMixSet* set = new mMixSet(setID, SetType::Mix, "contain", true);
        std::vector<RenderPickResultData> zhumianData;
        for (auto temp : zhumian)
        {
            RenderPickResultData tempdata;
            tempdata.result_id = temp.toInt();
            zhumianData.push_back(tempdata);
        }
        set->setMixSetData({ { MxFunctions::PickDataType::GeoFace,zhumianData } });
        getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set);
        mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_Connector, id }, { Physical_Set, setID }, MapRelation_Master);

        //创建从面内部集合
        setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
        set = new mMixSet(setID, SetType::Mix, "contain", true);
        std::vector<RenderPickResultData> congmianData;
        for (auto temp : congmian)
        {
            RenderPickResultData tempdata;
            tempdata.result_id = temp.toInt();
            congmianData.push_back(tempdata);
        }
        set->setMixSetData({ { MxFunctions::PickDataType::GeoFace,congmianData } });
        getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set);
        mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_Connector, id }, { Physical_Set, setID }, MapRelation_Slave);

        if (leixing.size() == 1)
        {
            if (leixing[0] == "点面绑定") Data->type_ = 0;
            if (leixing[0] == "面面绑定") Data->type_ = 1;
        }

        if (rongchaleixing.size() == 1)
        {
            if (rongchaleixing[0] == "自定义容差") Data->allowrancetype_ = 0;
            if (rongchaleixing[0] == "abaqus") Data->allowrancetype_ = 1;
            if (rongchaleixing[0] == "optistruct") Data->allowrancetype_ = 2;
        }

        if (rongcha.size() == 1) Data->allowrance_ = rongcha[0].toDouble();

        if (tiaozheng.size() == 1 && tiaozheng[0].toInt() == 1) Data->adjust_ = true;

        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建绑定 %1").arg(name[0]), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateConnectorSig(true, id);
        return true;
    }   
}