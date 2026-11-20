#pragma once
#include "mCommon_global.h"

#include "mPhysicalFieldEnum.h"
#include <QSet>

#include "mViewportModelManager.h"
#include "mViewportModel.h"

//Basic
#include "mMixSet.h"
#include "mMeshData.h"
#include "mGeoSet.h"
#include "mNode.h"
#include "mNodeFace.h"
#include "mElementSurface.h"
#include "mElementSet.h"
#include "mPartSet.h"
#include "mMapRelationManager.h"
#include "mSetManager.h"
#include "mModelData.h"

namespace MechMainWindow
{
    // 集合名 -> 按真实 SetType 分组数据（MixSet 展开内部真实类型；普通集合按自身类型归类）。
    // 作为类型化解析器注入 mPickWidgetController::setSetResolver，控制器不直接访问数据层。
    static std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>
        resolveSetToTypedData(const QString& setName)
    {
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> result;
        MechSetData::mSetBase * setBase = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setName);
        if (!setBase) return result;

        // 混合集合：直接返回内部 map（key 为各项真实类型），保留类型信息。
        if (auto* mixSet = dynamic_cast<MechSetData::mMixSet*>(setBase))
        {
            return mixSet->getMixSetData();
        }

        // 普通集合：按类型读取并按自身类型归类。
        switch (setBase->getSetType())
        {
        case MxFunctions::Node:
            if (auto* s = dynamic_cast<MechSetData::mNode*>(setBase)) result[MxFunctions::PickDataType::Node] = s->getNodeData();
            break;
        case MxFunctions::NodeSurface:
            // 节点面集：数据为节点，按节点类型归类（拾取类型仍为节点）
            if (auto* s = dynamic_cast<MechSetData::mNodeFace*>(setBase)) result[MxFunctions::PickDataType::Node] = s->getNodeData();
            break;
        case MxFunctions::Element:
            if (auto* s = dynamic_cast<MechSetData::mElementSet*>(setBase)) result[MxFunctions::PickDataType::Element] = s->getElementSetData();
            break;
        case MxFunctions::ElementSurface:
            if (auto* s = dynamic_cast<MechSetData::mElementSurface*>(setBase)) result[MxFunctions::PickDataType::ElementFace] = s->getElementSurfaceData();
            break;
        case MxFunctions::ElementEdge:
            if (auto* s = dynamic_cast<MechSetData::mElementSurface*>(setBase)) result[MxFunctions::PickDataType::ElementEdge] = s->getElementSurfaceData();
            break;
        case MxFunctions::GeoPoint:
        case MxFunctions::GeoEdge:
        case MxFunctions::GeoFace:
        case MxFunctions::GeoSolid:
            if (auto* s = dynamic_cast<MechSetData::mGeoSet*>(setBase))
            {
                std::vector<RenderPickResultData> geoResult;
                for (int id : s->getGeoSetData())
                    geoResult.emplace_back(id, -1, -1);
                result[SetTypeToPickDataType(setBase->getSetType())] = std::move(geoResult);
            }
            break;
        case MxFunctions::Part:
            if (auto* s = dynamic_cast<MechSetData::mPartSet*>(setBase))
            {
                std::vector<RenderPickResultData> partResult;
                for (int id : s->getPartSetData())
                    partResult.emplace_back(id, -1, -1);
                result[MxFunctions::PickDataType::Part] = std::move(partResult);
            }
            break;
        default:
            break;
        }
        return result;
    }

    inline QStringList getTarSet(QVector<MxFunctions::SetType>types)
    {
        QStringList tarsets;
        for (int i = 0; i < types.size(); ++i)
        {
            auto tarset = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(types[i]);
            tarsets.append(tarset);
        }
        return tarsets;
    }

    inline QHash<QString, QString> getSetNamesList(QVector<MxFunctions::SetType>types)
    {
        QStringList allSet = getTarSet(types);
        QHash<QString, QString> setToType;
        for (const auto& name : allSet)
        {
            auto* setBase = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(name);
            if (setBase)
            {
                setToType[name] = SetTypeToString(setBase->getSetType());
            }
            else
            {
                setToType[name] = "集合";
            }
        }
        return setToType;
    }

    inline QHash<QString, QString> getOneDataSet(const QHash<QString, QString>& setNames)
    {
        QHash<QString, QString> setToType;
        for (auto it = setNames.begin(); it != setNames.end(); ++it)
        {
            QString name = it.key();
            auto* setBase = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(name);
            switch (setBase->getSetType())
            {
            case MxFunctions::Node:
            {
                auto* s = dynamic_cast<MechSetData::mNode*>(setBase);
                if (s && s->getNodeData().size() == 1)
                {
                    setToType[name] = it.value();
                }
                break;
            }
            case MxFunctions::Element:
            {
                auto* s = dynamic_cast<MechSetData::mElementSet*>(setBase);
                if (s && s->getElementSetData().size() == 1)
                {
                    setToType[name] = it.value();
                }
                break;
            }
            case MxFunctions::ElementSurface:
            {
                auto* s = dynamic_cast<MechSetData::mElementSurface*>(setBase);
                if (s && s->getElementSurfaceData().size() == 1)
                {
                    setToType[name] = it.value();
                }
                break;
            }
            case MxFunctions::GeoPoint:
            case MxFunctions::GeoEdge:
            case MxFunctions::GeoFace:
            case MxFunctions::GeoSolid:
            {
                auto* s = dynamic_cast<MechSetData::mGeoSet*>(setBase);
                if (s && s->getGeoSetData().size() == 1)
                {
                    setToType[name] = it.value();
                }
                break;
            }
            case MxFunctions::Part:
            {
                auto* s = dynamic_cast<MechSetData::mPartSet*>(setBase);
                if (s && s->getPartSetData().size() == 1)
                {
                    setToType[name] = it.value();
                }
                break;
            }
            default:
                break;
            }
        }
        return setToType;
    }

}

