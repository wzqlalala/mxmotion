#include "IPickPersistenceStrategy.h"

#include "mSetBase.h"
#include "mSetManager.h"
#include "mMixSet.h"
#include "mNode.h"
#include "mDOFNode.h"
#include "mNodeFace.h"
#include "mElementSet.h"
#include "mElementSurface.h"
#include "mGeoSet.h"
#include "mPartSet.h"
#include "mMapRelationManager.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mContactManager.h"
#include "mContactBase.h"
#include "mAPIServer.h"
#include "mPythonAgent.h"

using namespace MechSetData;

namespace MechMainWindow
{
    // 閫氱敤锛氳幏鍙?Set 绠＄悊鍣紝渚?resolveSetBaseByName/Id 澶嶇敤銆?
    MechSetData::mSetManager& getSetManager()
    {
        return getCVM->getModelData()->getManager<MechSetData::mSetManager>(MxFunctions::Physical_Set);
    }

    mSetBase* resolveSetBaseByName(const QString& setName)
    {
        return getSetManager().getSetData(setName);
    }

    mSetBase* resolveSetBaseById(int setId)
    {
        return getSetManager().getSetData(setId);
    }

    void deleteInternalSetById(int setId)
    {
        getCVM->getModelData()->deleteByPhysicalType(MxFunctions::Physical_Set, setId);
    }

    void clearOldSetMapRelation(MxFunctions::PhysicalFieldType fieldType, int id, MxFunctions::MapRelationType mapType)
    {
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int oldSetId = mapRelation.getMapRelation<int>({ fieldType, id }, MxFunctions::Physical_Set, mapType);
        if (oldSetId != -1)
        {
            mSetBase* oldSet = resolveSetBaseById(oldSetId);
            if (oldSet && oldSet->getInternal())
            {
                deleteInternalSetById(oldSet->getSetId());
            }
            mapRelation.appendMapRelation({ fieldType, id }, { MxFunctions::Physical_Set, -1 }, mapType);
        }
    }

    void MapRelationPersistenceStrategy::save(
        const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data)
    {
        if (_idProvider < 0)
            return;

        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int oldSetId = mapRelation.getMapRelation<int>({ _fieldType, _idProvider }, MxFunctions::Physical_Set, _mapType);
        if (data.empty()) //濡傛灉data鏄┖ 琛ㄧず宸插皢鎷惧彇娓呯┖ 鐩存帴鍒犻櫎褰撳墠鏄犲皠鐨勯泦鍚?
        {
            // 清空拾取：仅解除本对象与集合的映射（并删除本对象独占的内部集合）。
            // 映射的集合可能是被多个载荷/边界共享的用户自建集合，不能在此删除，
            // 否则其他引用同一集合的面板再反显时集合已不存在，会被连带清空。
            clearOldSetMapRelation(_fieldType, _idProvider, _mapType);
            return;
        }
        mSetBase* oldSet = resolveSetBaseById(oldSetId);
        if (oldSet && oldSet->getInternal())
        {
            MechSetData::mMixSet* mixSet = dynamic_cast<mMixSet*>(oldSet);
            mixSet->setMixSetData(data);
        }
        else
        {
            int setID = getSetManager().createID();
            MechSetData::mMixSet* mixSet = new MechSetData::mMixSet(setID, MxFunctions::Mix, "contain", true);
            mixSet->setMixSetData(data);
            getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(mixSet);
            mapRelation.appendMapRelation({ _fieldType, _idProvider }, { MxFunctions::Physical_Set, setID }, _mapType);
        }
    }

    void MapRelationPersistenceStrategy::saveSingleSet(const QString& setName)
    {
        if (_idProvider < 0)
            return;

        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        clearOldSetMapRelation(_fieldType, _idProvider, _mapType);

        mSetBase* targetSet = resolveSetBaseByName(setName);
        if (targetSet)
        {
            mapRelation.appendMapRelation({ _fieldType, _idProvider }, { MxFunctions::Physical_Set, targetSet->getSetId() }, _mapType);
        }
    }

    // ============ APIPersistenceStrategy：拾取提交改走模块 API（当前支持 contact） ============

    QString APIPersistenceStrategy::serializePickData(
        const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data)
    {
        auto typePrefix = [](MxFunctions::PickDataType type) -> QString {
            switch (type)
            {
            case MxFunctions::PickDataType::GeoPoint:    return QStringLiteral("GeoPoint");
            case MxFunctions::PickDataType::GeoEdge:     return QStringLiteral("GeoEdge");
            case MxFunctions::PickDataType::GeoFace:     return QStringLiteral("GeoFace");
            case MxFunctions::PickDataType::GeoSolid:    return QStringLiteral("GeoSolid");
            case MxFunctions::PickDataType::Node:        return QStringLiteral("Node");
            case MxFunctions::PickDataType::ElementEdge: return QStringLiteral("ElementEdge");
            case MxFunctions::PickDataType::ElementFace: return QStringLiteral("ElementFace");
            case MxFunctions::PickDataType::Element:     return QStringLiteral("Element");
            case MxFunctions::PickDataType::Part:        return QStringLiteral("Part");
            default: return QString();
            }
        };

        QStringList groups;
        for (const auto& item : data)
        {
            const QString prefix = typePrefix(item.first);
            if (prefix.isEmpty() || item.second.empty())
                continue;

            // 单元/单元表面/单元边/部件需携带 part_id 和 result_id2："id@part_id:id2"
            const bool withExt = (item.first == MxFunctions::PickDataType::Element ||
                item.first == MxFunctions::PickDataType::ElementFace ||
                item.first == MxFunctions::PickDataType::ElementEdge ||
                item.first == MxFunctions::PickDataType::Part);
            QStringList ids;
            ids.reserve(static_cast<int>(item.second.size()));
            for (const auto& d : item.second)
            {
                if (withExt)
                    ids << QStringLiteral("%1@%2:%3").arg(d.result_id).arg(d.part_id).arg(d.result_id2);
                else
                    ids << QString::number(d.result_id);
            }
            groups << QStringLiteral("%1:%2").arg(prefix).arg(ids.join(QStringLiteral(",")));
        }
        return groups.join(QStringLiteral(";"));
    }

    bool APIPersistenceStrategy::resolveFieldMapping(QString& apiKey, QString& pySetter) const
    {
        if (_fieldType == MxFunctions::Physical_Contact)
        {
            if (_mapType == MxFunctions::MapRelation_Master)
            {
                apiKey = QStringLiteral("master");
                pySetter = QStringLiteral("set_master");
                return true;
            }
            if (_mapType == MxFunctions::MapRelation_Slave)
            {
                apiKey = QStringLiteral("slave");
                pySetter = QStringLiteral("set_slave");
                return true;
            }
        }
        return false;
    }

    QString APIPersistenceStrategy::resolveObjectName() const
    {
        if (_fieldType == MxFunctions::Physical_Contact)
        {
            auto& contactManager = getCVM->getModelData()->getManager<MechContactData::mContactManager>(MxFunctions::Physical_Contact);
            if (auto* contact = contactManager.getContactData(_idProvider))
                return contact->getContactName();
        }
        return QString();
    }

    void APIPersistenceStrategy::save(
        const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data)
    {
        if (_idProvider < 0)
            return;

        QString apiKey, pySetter;
        if (!resolveFieldMapping(apiKey, pySetter))
            return;
        const QString objectName = resolveObjectName();
        if (objectName.isEmpty())
            return;

        // 增量调用模块 API（空数据合法，表示清空该映射的集合数据）
        APIServer::APIParams params;
        params.insert(QStringLiteral("isCreate"), QStringList{ QStringLiteral("false") });
        params.insert(QStringLiteral("name"), QStringList{ objectName });
        params.insert(apiKey, QStringList{ serializePickData(data) });

        if (_fieldType == MxFunctions::Physical_Contact)
        {
            const QString cmd = QStringLiteral("Contact(\"%1\").%2(\"%3\").update()")
                .arg(objectName, pySetter, params.value(apiKey).first());
            // 先落盘再执行 API：闪退时肇事命令已在文件中；成功才入历史
            MechPreData::mPythonAgent::getInstance()->logCommandToFile(cmd);
            const auto apiResult = getAPISchedulingServer->executeAPI(QStringLiteral("contact"), params);
            if (apiResult._success)
                MechPreData::mPythonAgent::getInstance()->addCommandToHistory(cmd);
        }
    }

    void APIPersistenceStrategy::saveSingleSet(const QString& setName)
    {
        if (_idProvider < 0)
            return;

        QString apiKey, pySetter;
        if (!resolveFieldMapping(apiKey, pySetter))
            return;
        const QString objectName = resolveObjectName();
        if (objectName.isEmpty())
            return;

        // 引用已有集合：contact API 使用 masterSet/slaveSet 键
        APIServer::APIParams params;
        params.insert(QStringLiteral("isCreate"), QStringList{ QStringLiteral("false") });
        params.insert(QStringLiteral("name"), QStringList{ objectName });

        if (_fieldType == MxFunctions::Physical_Contact)
        {
            params.insert(apiKey + QStringLiteral("Set"), QStringList{ setName });
            const QString cmd = QStringLiteral("Contact(\"%1\").%2_set(\"%3\").update()")
                .arg(objectName, pySetter, setName);
            // 先落盘再执行 API：闪退时肇事命令已在文件中；成功才入历史
            MechPreData::mPythonAgent::getInstance()->logCommandToFile(cmd);
            const auto apiResult = getAPISchedulingServer->executeAPI(QStringLiteral("contact"), params);
            if (apiResult._success)
                MechPreData::mPythonAgent::getInstance()->addCommandToHistory(cmd);
        }
    }

    void SetDataPersistenceStrategy::save(
        const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data)
    {
        if (!_provider)
            return;
        mSetBase* set = _provider();
        if (!set)
            return;

        // 取指定类型的拾取数据（缺省为空，等效清空集合数据）
        auto dataOf = [&data](MxFunctions::PickDataType type) {
            auto it = data.find(type);
            return it != data.end() ? it->second : std::vector<RenderPickResultData>{};
        };
        // 几何/部件集合以 ID 列表存储
        auto idListOf = [&data](MxFunctions::PickDataType type) {
            std::vector<int> ids;
            auto it = data.find(type);
            if (it != data.end())
            {
                ids.reserve(it->second.size());
                for (const auto& d : it->second)
                    ids.push_back(d.result_id);
            }
            return ids;
        };

        // 按集合真实类型写入对应派生类
        switch (set->getSetType())
        {
        case MxFunctions::Node:
            if (auto* s = dynamic_cast<mNode*>(set)) s->setNodeData(dataOf(MxFunctions::PickDataType::Node));
            break;
        case MxFunctions::DOFNode:
            if (auto* s = dynamic_cast<mDOFNode*>(set)) s->setNodeData(dataOf(MxFunctions::PickDataType::Node));
            break;
        case MxFunctions::NodeSurface:
            if (auto* s = dynamic_cast<mNodeFace*>(set)) s->setNodeData(dataOf(MxFunctions::PickDataType::Node));
            break;
        case MxFunctions::Element:
            if (auto* s = dynamic_cast<mElementSet*>(set)) s->setElementSetData(dataOf(MxFunctions::PickDataType::Element));
            break;
        case MxFunctions::ElementSurface:
            if (auto* s = dynamic_cast<mElementSurface*>(set)) s->setElementSurfaceData(dataOf(MxFunctions::PickDataType::ElementFace));
            break;
        case MxFunctions::ElementEdge:
            if (auto* s = dynamic_cast<mElementSurface*>(set)) s->setElementSurfaceData(dataOf(MxFunctions::PickDataType::ElementEdge));
            break;
        case MxFunctions::GeoPoint:
        case MxFunctions::GeoEdge:
        case MxFunctions::GeoFace:
        case MxFunctions::GeoSolid:
            if (auto* s = dynamic_cast<mGeoSet*>(set))
                s->setGeoSetData(idListOf(SetTypeToPickDataType(set->getSetType())));
            break;
        case MxFunctions::Part:
            if (auto* s = dynamic_cast<mPartSet*>(set)) s->setPartSetData(idListOf(MxFunctions::PickDataType::Part));
            break;
        default:
            break;
        }
    }
}
