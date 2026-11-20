#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mMapRelationManager.h"

namespace MechMapRelation
{
    inline QDataStream& operator>>(QDataStream& in, mMapRelationManager& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammMapRelationManagerV3(in, mgr);
        }
        else
        {
            InStreammMapRelationManagerV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammMapRelationManagerV3(QDataStream& in, mMapRelationManager& mgr)
    {
        //if (FILE_VERSION >= 0x3000)
        //{
        //    
        //}
        //else
        //{
        InStreamMapRelationV3(in, mgr);
        //}
        return in;
    }



    inline QDataStream& InStreamMapRelationV3(QDataStream& in, mMapRelationManager& mgr)
    {
        in >> mgr._propertyToMat >> mgr._propertyToListMat >> mgr._propertyToShape >> mgr._propertyToListShape >> mgr._propertyToCoordinate
            >> mgr._propertyToLaminate >> mgr._propertyToDiscreteField;
        in >> mgr._partToProperty >> mgr._genericPartToListPart >> mgr._partMap;
        in >> mgr._loadBcToSet >> mgr._loadBcToCoordinate >> mgr._loadBcToFunction >> mgr._loadBcToCase >> mgr._loadBcToLoadBC
            >> mgr._loadToListLoad >> mgr._loadBcToListFunction >> mgr._loadBcToConnector;
        in >> mgr._loadBcToAnalysisField;
        in >> mgr._caseToFunction >> mgr._caseToCase >> mgr._caseToSet >> mgr._RandomCase >> mgr._caseToListLoadBC
            >> mgr._caseToListFreq >> mgr._caseToListNSM >> mgr._caseToListModelFix >> mgr._caseToListPeakFilter >> mgr._caseToGenericParam;
        in >> mgr._connectorToMat >> mgr._connectorToSet >> mgr._connectorToCoordinate;
        in >> mgr._deputyToCoordinate >> mgr._deputyToDeputy >> mgr._deputyToListPart;
        in >> mgr._predefinedFieldToSet >> mgr._predefinedFieldToFunction >> mgr._predefinedFieldToCase;
        in >> mgr._setToListSet;
        in >> mgr._coordinateToDiscreteField;
        in >> mgr._contactToSet >> mgr._contactToContact >> mgr._contactToCase >> mgr._contactToFunction;
        in >> mgr._rotateToPart >> mgr._rotateToCase;
        in >> mgr._periodicBoundaryToSet >> mgr._modelFixToSet;
        in >> mgr._laminateToMat >> mgr._laminateToSet >> mgr._laminateToDiscreteField >> mgr._laminateToLayerList;
        in >> mgr._goalToCase >> mgr._goalToListCase >> mgr._designToProperty >> mgr._designToSet >> mgr._designToDesign >> mgr._constraintToProperty >> mgr._constraintToCase;
        in >> mgr._modalContriToSet >> mgr._nodeContriToSet >> mgr._tpContriToSet >> mgr._panelContriToSet;
        in >> mgr._microconstituentToMat >> mgr._microconstituentToFunction;
        in >> mgr._fieldToCoordinate >> mgr._fieldToSet;

        in >> mgr._setMap >> mgr._coordinateMap >> mgr._functionMap >> mgr._caseMap >> mgr._propertyMap >> mgr._matMap
            >> mgr._propertyShapeMap >> mgr._loadBcMap >> mgr._connectorMap >> mgr._freqMap >> mgr._laminateMap >> mgr._nsmMap
            >> mgr._modelFixMap >> mgr._peakFilterMap >> mgr._genericParamMap >> mgr._deputyMap >> mgr._discreteFieldMap >> mgr._contactMap;

        //多体实体引用映射（嵌套结构手动反序列化，与写出侧对称）
        qint32 srcTypeCount = 0;
        in >> srcTypeCount;
        for (qint32 i = 0; i < srcTypeCount; ++i)
        {
            qint32 srcType = 0; qint32 srcCount = 0;
            in >> srcType >> srcCount;
            for (qint32 j = 0; j < srcCount; ++j)
            {
                qint32 srcID = 0; qint32 roleCount = 0;
                in >> srcID >> roleCount;
                for (qint32 k = 0; k < roleCount; ++k)
                {
                    qint32 dstType = 0; qint32 role = 0; qint32 dstCount = 0;
                    in >> dstType >> role >> dstCount;
                    std::set<int> ids;
                    for (qint32 m = 0; m < dstCount; ++m)
                    {
                        qint32 v = 0;
                        in >> v;
                        ids.insert((int)v);
                    }
                    if (!ids.empty())
                        mgr._motionRefMap[(MxFunctions::PhysicalFieldType)srcType][(int)srcID][
                            { (MxFunctions::PhysicalFieldType)dstType, (MxFunctions::MapRelationType)role }] = ids;
                }
            }
        }
        return in;
    }
}


