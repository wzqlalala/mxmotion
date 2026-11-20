#include "mContactAPIRegistrar.h"

#include "mPhysicalFieldEnum.h"
#include "mContactManager.h"
#include "mContactBase.h"
#include "mFunctionManager.h"
#include "mCaseManager.h"
#include "mMixSet.h"
#include "mSetManager.h"
#include "mModelData.h"
#include "mMeshData.h"
#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"
#include "mDataStatusManager.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"

using namespace MechContactData;
using namespace MechSetData;
using namespace MxFunctions;
using namespace MechCommon;

namespace MechMainWindow
{
    namespace
    {
        // ============ contact API 字符串解析工具（Python→C++ 全字符串传参，这里统一转换） ============
        QString contactParamStr(const APIServer::APIParams& params, const QString& key)
        {
            const QStringList values = params.value(key);
            return values.isEmpty() ? QString() : values.first();
        }

        bool contactParamToBool(const QString& value)
        {
            return value == QStringLiteral("true") || value == QStringLiteral("1") || value == QStringLiteral("开启");
        }

        bool contactParamIsTrue(const APIServer::APIParams& params, const QString& key)
        {
            return contactParamToBool(contactParamStr(params, key));
        }

        double contactParamToDouble(const QString& value, double defaultValue = 0.0)
        {
            bool ok = false;
            const double result = value.toDouble(&ok);
            return ok ? result : defaultValue;
        }

        int contactParamToInt(const QString& value, int defaultValue = 0)
        {
            bool ok = false;
            const int result = value.toInt(&ok);
            return ok ? result : defaultValue;
        }

        // 热接触间隙传导表：["导热系数:间隙值", ...] → QList<QPair<double,double>>
        QList<QPair<double, double>> contactParamToPairList(const QStringList& values)
        {
            QList<QPair<double, double>> result;
            for (const QString& item : values)
            {
                const QStringList pair = item.split(QStringLiteral(":"));
                const double first = pair.isEmpty() ? 0.0 : contactParamToDouble(pair.at(0));
                const double second = pair.size() < 2 ? 0.0 : contactParamToDouble(pair.at(1));
                result.append({ first, second });
            }
            return result;
        }

        // 接触类型字符串 → 枚举
        bool contactTypeFromString(const QString& typeStr, MxFunctions::ContactType& type)
        {
            if (typeStr == QStringLiteral("面面接触")) { type = MxFunctions::SurfaceToSurfaceContact; return true; }
            if (typeStr == QStringLiteral("点面接触")) { type = MxFunctions::NodeToSurfaceContact; return true; }
            if (typeStr == QStringLiteral("通用接触")) { type = MxFunctions::CommonContact; return true; }
            if (typeStr == QStringLiteral("自接触"))   { type = MxFunctions::SelfContact; return true; }
            if (typeStr == QStringLiteral("过盈接触")) { type = MxFunctions::InterferenceContact; return true; }
            if (typeStr == QStringLiteral("二维接触")) { type = MxFunctions::TwoDimensionalContact; return true; }
            if (typeStr == QStringLiteral("线面接触")) { type = MxFunctions::LineToSurfaceContact; return true; }
            return false;
        }

        // PickDataType 字符串 → 枚举（通用集合接口用，支持所有拾取类型）
        bool pickDataTypeFromString(const QString& typeStr, MxFunctions::PickDataType& type)
        {
            if (typeStr == QStringLiteral("GeoPoint"))     { type = MxFunctions::PickDataType::GeoPoint; return true; }
            if (typeStr == QStringLiteral("GeoEdge"))      { type = MxFunctions::PickDataType::GeoEdge; return true; }
            if (typeStr == QStringLiteral("GeoFace"))      { type = MxFunctions::PickDataType::GeoFace; return true; }
            if (typeStr == QStringLiteral("GeoSolid"))     { type = MxFunctions::PickDataType::GeoSolid; return true; }
            if (typeStr == QStringLiteral("Node"))         { type = MxFunctions::PickDataType::Node; return true; }
            if (typeStr == QStringLiteral("ElementEdge"))  { type = MxFunctions::PickDataType::ElementEdge; return true; }
            if (typeStr == QStringLiteral("ElementFace"))  { type = MxFunctions::PickDataType::ElementFace; return true; }
            if (typeStr == QStringLiteral("Element"))      { type = MxFunctions::PickDataType::Element; return true; }
            if (typeStr == QStringLiteral("Part"))         { type = MxFunctions::PickDataType::Part; return true; }
            return false;
        }

        // 通用ID字符串 → 直接构建 mMixSet 数据格式
        // 格式："GeoFace:1,2,3;Node:5,6"
        // 单元/单元表面等支持扩展字段："Element:10@1:0,11@1:0"（id@part_id:id2）
        bool parsePickIdsString(const QStringList& ids,
            std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& out,
            QString& error)
        {
            for (const QString& item : ids)
            {
                const QString trimmed = item.trimmed();
                if (trimmed.isEmpty()) continue;

                // 按分号拆分类型组："GeoFace:1,2,3;Node:5,6"
                const QStringList typeGroups = trimmed.split(QStringLiteral(";"), QString::SkipEmptyParts);
                for (const QString& group : typeGroups)
                {
                    const int colonIdx = group.indexOf(QStringLiteral(":"));
                    if (colonIdx <= 0)
                    {
                        error = QStringLiteral("集合ID格式错误，缺少类型前缀: %1").arg(group);
                        return false;
                    }

                    // 有类型前缀："GeoFace:1,2,3"
                    const QString typeStr = group.left(colonIdx);
                    const QString idsStr = group.mid(colonIdx + 1);

                    MxFunctions::PickDataType pickType;
                    if (!pickDataTypeFromString(typeStr, pickType))
                    {
                        error = QStringLiteral("未识别的拾取类型: %1").arg(typeStr);
                        return false;
                    }

                    // 按逗号拆分ID，支持扩展格式 "id@part_id:id2"
                    const QStringList idStrs = idsStr.split(QStringLiteral(","), QString::SkipEmptyParts);
                    for (const QString& idStr : idStrs)
                    {
                        RenderPickResultData data;

                        // 解析扩展字段："10@1:0" → id=10, part_id=1, id2=0
                        QString idPart = idStr;
                        const int atIdx = idStr.indexOf(QStringLiteral("@"));
                        if (atIdx > 0)
                        {
                            idPart = idStr.left(atIdx);
                            const QString extStr = idStr.mid(atIdx + 1);
                            const int colon2Idx = extStr.indexOf(QStringLiteral(":"));
                            if (colon2Idx > 0)
                            {
                                data.part_id = extStr.left(colon2Idx).toInt();
                                data.result_id2 = extStr.mid(colon2Idx + 1).toInt();
                            }
                            else
                            {
                                data.part_id = extStr.toInt();
                            }
                        }

                        data.result_id = idPart.toInt();

                        // 单元类型：无 part_id 时自动查找部件ID
                        if (pickType == MxFunctions::PickDataType::Element && data.part_id == -1)
                        {
                            auto* meshData = getCVM->getModelData()->getMeshData();
                            if (meshData)
                                data.part_id = meshData->getPartIdByEleId(data.result_id);
                        }

                        // 单元表面/单元边：缺少 part_id 或 result_id2 时报错跳过
                        if ((pickType == MxFunctions::PickDataType::ElementFace ||
                             pickType == MxFunctions::PickDataType::ElementEdge) &&
                            (data.part_id == -1 || data.result_id2 == -1))
                        {
                            error = QStringLiteral("%1 需要 part_id 和 id2，格式应为 \"id@part_id:id2\"，错误ID: %2")
                                .arg(typeStr).arg(idStr);
                            return false;
                        }

                        out[pickType].push_back(data);
                    }
                }
            }
            return true;
        }
    }

    // 全局单例指针
    static mContactAPIRegistrar* g_contactAPIRegistrar = nullptr;
    mContactAPIRegistrar* mContactAPIRegistrar::instance()
    {
        if (!g_contactAPIRegistrar) {
            g_contactAPIRegistrar = new mContactAPIRegistrar();
        }
        return g_contactAPIRegistrar;
    }

    void mContactAPIRegistrar::registerContactAPIs()
    {
        // 注册 API Server 回调：contact -> onContactAPI（JSON全字符串参数，isCreate 标志位区分创建/更新）
        getAPISchedulingServer->registerAPI("contact", this, &mContactAPIRegistrar::onContactAPI,
            "接触统一入口（创建/更新）");
    }

    void mContactAPIRegistrar::unregisterContactAPIs()
    {
        getAPISchedulingServer->unregisterAPI("contact");
    }

    // 通用集合接口：更新或创建指定映射关系的内部集合数据（支持所有 PickDataType）
    bool mContactAPIRegistrar::updateContactSetData(int contactID, MxFunctions::MapRelationType relationType,
        const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data,
        QString& error)
    {
        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        auto& setManager = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set);

        // 更新分支：若该映射已有内部集合则复用，避免每次增量提交都新建集合导致泄漏
        int setID = -1;
        MechSetData::mMixSet* set = nullptr;
        const int existingSetId = mapRelationManager.getMapRelation<int>(
            { PhysicalFieldType::Physical_Contact, contactID }, Physical_Set, relationType);
        if (existingSetId != -1)
        {
            auto* base = setManager.getSetData(existingSetId);
            if (base && base->getSetType() == SetType::Mix)
            {
                set = dynamic_cast<MechSetData::mMixSet*>(base);
                setID = existingSetId;
            }
        }

        if (!set)
        {
            setID = setManager.createID();
            set = new MechSetData::mMixSet(setID, SetType::Mix, "contain", true);
            getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set);
        }

        set->setMixSetData(data);
        mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_Contact, contactID }, { Physical_Set, setID }, relationType);
        return true;
    }

    // 通用集合接口：把指定映射关系改为引用已有集合
    bool mContactAPIRegistrar::linkContactSet(int contactID, MxFunctions::MapRelationType relationType,
        const QString& setName, QString& error)
    {
        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        auto& setManager = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set);

        const int setID = setManager.getIDBySetName(setName);
        if (setID == -1)
        {
            error = QStringLiteral("未找到集合: %1").arg(setName);
            return false;
        }

        mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_Contact, contactID }, { Physical_Set, setID }, relationType);
        return true;
    }

    // 接触统一入口：JSON全字符串参数，isCreate="true"/"false" 区分创建/更新
    QVariant mContactAPIRegistrar::onContactAPI(const APIServer::APIParams& params, QString& error)
    {
        auto& contactManager = getCVM->getModelData()->getManager<mContactManager>(Physical_Contact);
        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        const bool isCreate = contactParamIsTrue(params, QStringLiteral("isCreate"));

        // ---- 解析通用字符串参数（缺省为空串表示"未提供"） ----
        const QString name = contactParamStr(params, QStringLiteral("name"));
        const QString typeStr = contactParamStr(params, QStringLiteral("type"));
        QStringList master = params.value(QStringLiteral("master"));
        QStringList slave = params.value(QStringLiteral("slave"));
        const bool hasMaster = params.contains(QStringLiteral("master"));
        const bool hasSlave = params.contains(QStringLiteral("slave"));

        MechContactData::mContactBase* contact = nullptr;
        int contactID = -1;

        if (isCreate)
        {
            // ============ 创建分支 ============
            if (contactManager.ExistContactName(name))
            {
                error = QStringLiteral("名称已存在");
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QStringLiteral("名称已存在，接触创建失败！"));
                return false;
            }

            MxFunctions::ContactType contactType = MxFunctions::SurfaceToSurfaceContact;
            if (!typeStr.isEmpty() && !contactTypeFromString(typeStr, contactType))
            {
                error = QStringLiteral("未识别的接触类型: %1").arg(typeStr);
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QStringLiteral("未识别的接触类型，接触创建失败！"));
                return false;
            }

            contactID = contactManager.createID();
            contact = new MechContactData::mContactBase(contactID, contactType);
            if (!name.isEmpty())
            {
                contact->setContactName(name, true);
            }
            getCVM->getModelData()->addPhysicalField<MechContactData::mContactBase>(contact);
        }
        else
        {
            if (name.isEmpty())
            {
                error = QStringLiteral("未识别到名称");
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QStringLiteral("未识别到名称，接触更新失败！"));
                return false;
            }

            // ============ 更新分支：定位已有接触 ============
            contact = contactManager.getContactData(name);
            if (!contact)
            {
                error = QStringLiteral("未找到接触: %1").arg(name);
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QStringLiteral("未找到接触 %1，接触更新失败！").arg(name));
                return false;
            }
            contactID = contact->getContactId();
        }

        // ---- 类型切换处理：更新分支且传了 type 键时，按新类型校验并重置不兼容字段 ----
        if (!isCreate && params.contains(QStringLiteral("type")))
        {
            MxFunctions::ContactType newType;
            if (contactTypeFromString(typeStr, newType) && contact->getContactType() != newType)
            {
                const auto oldType = contact->getContactType();
                contact->setContactType(newType);

                const auto isPairContact = [](MxFunctions::ContactType type) {
                    return type == MxFunctions::SurfaceToSurfaceContact
                        || type == MxFunctions::NodeToSurfaceContact
                        || type == MxFunctions::TwoDimensionalContact
                        || type == MxFunctions::LineToSurfaceContact;
                    };
                if (isPairContact(oldType) && isPairContact(newType))
                {
                    QString clearError;
                    updateContactSetData(contactID, MapRelation_Master, {}, clearError);
                    updateContactSetData(contactID, MapRelation_Slave, {}, clearError);
                }

                // 离开点面/线面接触时剥离主/从面集合中的节点数据：节点/节点面集仅这两类接触的从面合法，
                // 切到面面等类型后集合与通用拾取控件不得残留节点拾取
                if (newType != MxFunctions::NodeToSurfaceContact
                    && newType != MxFunctions::LineToSurfaceContact)
                {
                    auto& setManager = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set);
                    auto stripNodeData = [&](MxFunctions::MapRelationType relationType)
                        {
                            int setId = mapRelationManager.getMapRelation<int>(
                                { PhysicalFieldType::Physical_Contact, contactID }, Physical_Set, relationType);
                            if (setId == -1) return;
                            mSetBase* setBase = setManager.getSetData(setId);
                            if (!setBase) return;

                            if (setBase->getSetType() == SetType::Mix)
                            {
                                // 内部混合集合：仅剔除节点项后回写，面数据保留
                                auto* mixSet = dynamic_cast<MechSetData::mMixSet*>(setBase);
                                if (!mixSet) return;
                                auto mixData = mixSet->getMixSetData();
                                auto nodeIt = mixData.find(MxFunctions::PickDataType::Node);
                                if (nodeIt == mixData.end() || nodeIt->second.empty()) return;
                                mixData.erase(nodeIt);
                                mixSet->setMixSetData(mixData);
                            }
                            else if (setBase->getSetType() == MxFunctions::Node
                                || setBase->getSetType() == MxFunctions::NodeSurface)
                            {
                                // 引用的用户节点/节点面集合：不动用户集合本身，映射改为空内部集合
                                QString stripError;
                                updateContactSetData(contactID, relationType, {}, stripError);
                            }
                        };
                    stripNodeData(MapRelation_Master);
                    stripNodeData(MapRelation_Slave);
                }
            }
        }

        // ---- 主/从面：支持两种方式 ----
        // 1. master/slave 键：通用ID字符串（"GeoFace:1,2,3;Node:5,6"）
        // 2. masterSet/slaveSet 键：已有集合名称，直接映射原有集合
        const QString masterSetName = contactParamStr(params, QStringLiteral("masterSet"));
        const QString slaveSetName = contactParamStr(params, QStringLiteral("slaveSet"));

        // 方式2：直接映射已有集合
        if (!masterSetName.isEmpty())
        {
            if (!linkContactSet(contactID, MapRelation_Master, masterSetName, error))
                return false;
        }
        else if (hasMaster)
        {
            // 方式1：通用ID字符串（支持空集合）
            std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> setData;
            if (!parsePickIdsString(master, setData, error))
                return false;
            if (!updateContactSetData(contactID, MapRelation_Master, setData, error))
                return false;
        }

        if (!slaveSetName.isEmpty())
        {
            if (!linkContactSet(contactID, MapRelation_Slave, slaveSetName, error))
                return false;
        }
        else if (hasSlave)
        {
            // 方式1：通用ID字符串（支持空集合）
            std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> setData;
            if (!parsePickIdsString(slave, setData, error))
                return false;
            if (!updateContactSetData(contactID, MapRelation_Slave, setData, error))
                return false;
        }

        // ---- 过盈接触映射字段：接触对/函数/分析步（按名称引用已有对象） ----
        if (params.contains(QStringLiteral("contactPair")))
        {
            const QString pairName = contactParamStr(params, QStringLiteral("contactPair"));
            const int pairId = contactManager.getContactIdByName(pairName);
            auto* pairContact = contactManager.getContactData(pairId);
            if (!pairContact)
            {
                error = QStringLiteral("未找到接触对: %1").arg(pairName);
                return false;
            }
            if (contact->getContactType() == MxFunctions::InterferenceContact)
            {
                const auto pairType = pairContact->getContactType();
                const bool isSupported = pairType == MxFunctions::SurfaceToSurfaceContact
                    || pairType == MxFunctions::NodeToSurfaceContact
                    || pairType == MxFunctions::TwoDimensionalContact;
                if (!isSupported || pairContact->isSuppressed())
                {
                    error = QStringLiteral("过盈接触的接触对必须是未抑制的面面、点面或二维接触: %1").arg(pairName);
                    return false;
                }
            }
            mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_Contact, contactID }, { Physical_Contact, pairId });
        }
        if (params.contains(QStringLiteral("function")))
        {
            const QString funcName = contactParamStr(params, QStringLiteral("function"));
            auto& funcManager = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function);
            const int funcId = funcManager.getIDByFunctionName(funcName);
            if (funcId == -1)
            {
                error = QStringLiteral("未找到函数: %1").arg(funcName);
                return false;
            }
            mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_Contact, contactID }, { Physical_Function, funcId });
        }
        if (params.contains(QStringLiteral("analysisStep")))
        {
            const QString caseName = contactParamStr(params, QStringLiteral("analysisStep"));
            auto& caseManager = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case);
            const int caseId = caseManager.getIDByCaseName(caseName);
            if (caseId == -1)
            {
                error = QStringLiteral("未找到分析步: %1").arg(caseName);
                return false;
            }
            // 与 mTreeView 一致：接触(过盈) -> 工况，默认角色 MapRelation_None
            mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_Contact, contactID },
                { Physical_Case, caseId });
        }
        if (params.contains(QStringLiteral("analysisStepRemove")))
        {
            // 解除接触与指定分析步的映射
            const QString caseName = contactParamStr(params, QStringLiteral("analysisStepRemove"));
            auto& caseManager = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case);
            const int caseId = caseManager.getIDByCaseName(caseName);
            if (caseId == -1)
            {
                error = QStringLiteral("未找到分析步: %1").arg(caseName);
                return false;
            }
            // 解除映射走 Case->Contact 分派（与 mTreeView/mManagerTableWidget 既有删除一致）
            mapRelationManager.deleteMapRelation({ Physical_Case, caseId }, { PhysicalFieldType::Physical_Contact, contactID });
        }

        // ---- 交换主/从面映射（直接交换映射的集合ID，不重建集合数据） ----
        if (contactParamIsTrue(params, QStringLiteral("swap")))
        {
            const int masterSetId = mapRelationManager.getMapRelation<int>(
                { PhysicalFieldType::Physical_Contact, contactID }, Physical_Set, MapRelation_Master);
            const int slaveSetId = mapRelationManager.getMapRelation<int>(
                { PhysicalFieldType::Physical_Contact, contactID }, Physical_Set, MapRelation_Slave);
            if (masterSetId != -1 || slaveSetId != -1)
            {
                mapRelationManager.appendMapRelation(
                    { PhysicalFieldType::Physical_Contact, contactID },
                    { Physical_Set, slaveSetId }, MapRelation_Master);
                mapRelationManager.appendMapRelation(
                    { PhysicalFieldType::Physical_Contact, contactID },
                    { Physical_Set, masterSetId }, MapRelation_Slave);
            }
        }

        // ---- 通用字段：创建分支全量按提供值覆盖；更新分支只处理出现的键 ----
        auto hasKey = [&](const QString& key) { return isCreate || params.contains(key); };

        if (hasKey(QStringLiteral("masterDir")))
            contact->masterDir = !params.contains(QStringLiteral("masterDir")) || contactParamIsTrue(params, QStringLiteral("masterDir"));
        if (hasKey(QStringLiteral("slaveDir")))
            contact->slaveDir = !params.contains(QStringLiteral("slaveDir")) || contactParamIsTrue(params, QStringLiteral("slaveDir"));
        if (hasKey(QStringLiteral("slaveRegion")))
            contact->slave_region = contactParamToInt(contactParamStr(params, QStringLiteral("slaveRegion")));
        if (hasKey(QStringLiteral("smallSlip")))
            contact->smallSlip = contactParamIsTrue(params, QStringLiteral("smallSlip"));
        if (hasKey(QStringLiteral("selfContactSearch")))
            contact->selfContactSearch = contactParamIsTrue(params, QStringLiteral("selfContactSearch"));

        if (hasKey(QStringLiteral("overclosure")))
        {
            const QString v = contactParamStr(params, QStringLiteral("overclosure"));
            if (!v.isEmpty()) contact->overclosure = v;
        }
        if (contact->getContactType() == MxFunctions::TwoDimensionalContact
            && contact->overclosure != QStringLiteral("硬接触")
            && contact->overclosure != QStringLiteral("线性接触"))
        {
            contact->overclosure = QStringLiteral("硬接触");
        }
        if (hasKey(QStringLiteral("constraint")))
        {
            const QString v = contactParamStr(params, QStringLiteral("constraint"));
            if (!v.isEmpty()) contact->constraint = v;
        }
        if (hasKey(QStringLiteral("scaleFactor")))
        {
            const QString v = contactParamStr(params, QStringLiteral("scaleFactor"));
            if (!v.isEmpty()) contact->scale_factor = v;
        }
        if (hasKey(QStringLiteral("penaltyFactor")))
        {
            const QString v = contactParamStr(params, QStringLiteral("penaltyFactor"));
            if (!v.isEmpty()) contact->penalty_factor = v;
        }

        if (hasKey(QStringLiteral("tangentialDefine")))
            contact->tangential_define = contactParamIsTrue(params, QStringLiteral("tangentialDefine"));
        if (hasKey(QStringLiteral("tangentialFriction")))
        {
            const QString v = contactParamStr(params, QStringLiteral("tangentialFriction"));
            if (!v.isEmpty()) contact->tangential_friction = v;
        }
        if (hasKey(QStringLiteral("tangentialStaticCoef")))
            contact->tangential_static_coef = contactParamToDouble(contactParamStr(params, QStringLiteral("tangentialStaticCoef")));

        if (contact->getContactType() == MxFunctions::TwoDimensionalContact
            && contact->tangential_friction != QStringLiteral("无摩擦")
            && contact->tangential_friction != QStringLiteral("通用接触摩擦"))
        {
            contact->tangential_friction = QStringLiteral("无摩擦");
        }

        if (hasKey(QStringLiteral("heatDefine")))
            contact->heat_define = contactParamIsTrue(params, QStringLiteral("heatDefine"));
        if (hasKey(QStringLiteral("heatValue")) && params.contains(QStringLiteral("heatValue")))
            contact->heat_value = contactParamToPairList(params.value(QStringLiteral("heatValue")));

        if (hasKey(QStringLiteral("gapDefine")))
            contact->gap_define = contactParamIsTrue(params, QStringLiteral("gapDefine"));
        if (hasKey(QStringLiteral("gapOverclosure")))
            contact->gap_overclosure = contactParamToDouble(contactParamStr(params, QStringLiteral("gapOverclosure")));

        if (hasKey(QStringLiteral("geomDefine")))
            contact->geom_define = contactParamIsTrue(params, QStringLiteral("geomDefine"));
        if (hasKey(QStringLiteral("geomValue")))
            contact->geom_value = contactParamToDouble(contactParamStr(params, QStringLiteral("geomValue")));

        if (hasKey(QStringLiteral("clrFitType")))
            contact->clr_fit_typ = contactParamToInt(contactParamStr(params, QStringLiteral("clrFitType")));
        if (hasKey(QStringLiteral("size")))
            contact->size = contactParamStr(params, QStringLiteral("size"));

        if (hasKey(QStringLiteral("evaluation")))
            contact->evaluation = contactParamIsTrue(params, QStringLiteral("evaluation"));
        if (hasKey(QStringLiteral("evaMP")))
            contact->eva_MP = contactParamIsTrue(params, QStringLiteral("evaMP"));
        if (hasKey(QStringLiteral("evaSD")))
            contact->eva_SD = contactParamIsTrue(params, QStringLiteral("evaSD"));
        if (hasKey(QStringLiteral("evaCV")))
            contact->eva_CV = contactParamIsTrue(params, QStringLiteral("evaCV"));
        if (hasKey(QStringLiteral("evaPC")))
            contact->eva_PC = contactParamIsTrue(params, QStringLiteral("evaPC"));
        if (hasKey(QStringLiteral("evaR")))
            contact->eva_R = contactParamIsTrue(params, QStringLiteral("evaR"));

        // ---- 通知刷新 ----
        if (isCreate)
        {
            // 创建时发信号，由 mTreeView::slot_updateContactTree 统一处理（内部已含 updateDynamicInfoData）
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateContactSig(true, contactID);
            mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建接触 %1").arg(contact->getContactName()), true);
        }
        else
        {
            // 更新时直接刷新数据状态
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Contact, contactID);
        }

        return true;
    }
}
