#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include <vector>

#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mMapRelationManager.h"

namespace MechMapRelation
{
    inline QDataStream& operator<<(QDataStream& out, mMapRelationManager& mgr)
    {
        out << mgr._propertyToMat << mgr._propertyToListMat << mgr._propertyToShape << mgr._propertyToListShape<< mgr._propertyToCoordinate
            << mgr._propertyToLaminate << mgr._propertyToDiscreteField;
        out << mgr._partToProperty << mgr._genericPartToListPart << mgr._partMap;
        out << mgr._loadBcToSet << mgr._loadBcToCoordinate << mgr._loadBcToFunction << mgr._loadBcToCase << mgr._loadBcToLoadBC
            << mgr._loadToListLoad << mgr._loadBcToListFunction << mgr._loadBcToConnector;
        out << mgr._loadBcToAnalysisField;
        out << mgr._caseToFunction << mgr._caseToCase << mgr._caseToSet << mgr._RandomCase << mgr._caseToListLoadBC
            << mgr._caseToListFreq << mgr._caseToListNSM << mgr._caseToListModelFix << mgr._caseToListPeakFilter << mgr._caseToGenericParam;
        out << mgr._connectorToMat << mgr._connectorToSet << mgr._connectorToCoordinate;
        out << mgr._deputyToCoordinate << mgr._deputyToDeputy << mgr._deputyToListPart;
        out << mgr._predefinedFieldToSet << mgr._predefinedFieldToFunction << mgr._predefinedFieldToCase;
        out << mgr._setToListSet;
        out << mgr._coordinateToDiscreteField;
        out << mgr._contactToSet << mgr._contactToContact << mgr._contactToCase << mgr._contactToFunction;
        out << mgr._rotateToPart << mgr._rotateToCase;
        out << mgr._periodicBoundaryToSet << mgr._modelFixToSet;
        out << mgr._laminateToMat << mgr._laminateToSet << mgr._laminateToDiscreteField << mgr._laminateToLayerList;
        out << mgr._goalToCase << mgr._goalToListCase << mgr._designToProperty << mgr._designToSet << mgr._designToDesign << mgr._constraintToProperty << mgr._constraintToCase;
        out << mgr._modalContriToSet << mgr._nodeContriToSet << mgr._tpContriToSet << mgr._panelContriToSet;
        out << mgr._microconstituentToMat << mgr._microconstituentToFunction;
        out << mgr._fieldToCoordinate << mgr._fieldToSet;

        out << mgr._setMap << mgr._coordinateMap << mgr._functionMap << mgr._caseMap << mgr._propertyMap << mgr._matMap
            << mgr._propertyShapeMap << mgr._loadBcMap << mgr._connectorMap << mgr._freqMap << mgr._laminateMap << mgr._nsmMap
            << mgr._modelFixMap << mgr._peakFilterMap << mgr._genericParamMap << mgr._deputyMap << mgr._discreteFieldMap << mgr._contactMap;

        //多体实体引用映射（嵌套结构手动序列化：源类型->源ID->角色->目标集合）
        out << (qint32)mgr._motionRefMap.size();
        for (const auto& t2map : mgr._motionRefMap)
        {
            out << (qint32)t2map.first << (qint32)t2map.second.size();
            for (const auto& id2roles : t2map.second)
            {
                out << (qint32)id2roles.first << (qint32)id2roles.second.size();
                for (const auto& role2set : id2roles.second)
                {
                    out << (qint32)role2set.first.first << (qint32)role2set.first.second << (qint32)role2set.second.size();
                    for (int v : role2set.second)
                        out << (qint32)v;
                }
            }
        }
        return out;
    }
}

