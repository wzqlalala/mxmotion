#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include <vector>

#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mStructOption.h"
#include "mOptimizationConstraintData.h"
#include "mOptimizationDesignData.h"
#include "mOptimizationGoalData.h"


namespace MechStructOptionData
{
    // ==============================================================================
     // mStructOptionData 序列化
     // ==============================================================================
    inline QDataStream& operator<<(QDataStream& out, mStructOptionData& obj)
    {
        out << obj._isMaxiter << obj._maxiter << obj._isObjectivetolrance << obj._objectivetolrance
            << obj._isDiscretetarget << obj._discretetarget << obj._isTopomovelimit << obj._topomovelimit
            << obj._isRmin << obj._rmin << obj._isPenal << obj._penal << obj._isInitdensity << obj._initdensity
            << obj._isMinDesity << obj._minDesity << obj._isChecker << obj._checker << obj._isMatintertype << obj._matintertype
            << obj._isRebuild << obj._rebuild
            << obj._isRestart << obj._iteration << obj._resStartPath;;
       return out;
    }



    // ==============================================================================
    // mOptimizationGoalData 序列化
    // ==============================================================================
    inline QDataStream& operator<<(QDataStream& out, mOptimizationGoalData& obj)
    {
        out << obj._objectiveType << obj._responseType << obj._frequencyOrder << obj._isCompliance
            << obj._isFrequency << obj._caseToweight << obj._order << obj._node
            << obj._nodeDofs << obj._frequencyTargetType << obj._specifiedFrequency << obj._frfValueType;
        return out;
    }

    // ==============================================================================
    // mOptimizationConstraintData 序列化
    // ==============================================================================
    inline QDataStream& operator<<(QDataStream& out, mOptimizationConstraintData& obj)
    {
        // 基本属性
        out << obj._id
            << obj._name
            << obj._describe;

        // 优化约束
        out << obj._upperBoundStatus << obj._upperBoundValue
            << obj._lowerBoundStatus << obj._lowerBoundValue
            << obj._constraintType;

        // 指定区域
        out << obj._isArea;

        // 响应类型参数(与优化目标一致)
        out << obj._frequencyOrder << obj._isCompliance << obj._isFrequency
            << obj._caseToweight << obj._order << obj._node
            << obj._nodeDofs << obj._frequencyTargetType << obj._specifiedFrequency
            << obj._frfValueType;

        return out;
    }

    // ==============================================================================
    // mOptimizationDesignData 序列化
    // ==============================================================================
        inline QDataStream& operator<<(QDataStream& out, mOptimizationDesignData& obj)
    {
        // 1. 基本属性
        out <<  obj._id <<  obj._name <<  obj._describe;

        // 2. 新增：设计域属性
        out <<  obj._designDomainType
            <<  obj._optimizationMethod
            <<  obj._baseThickness;

        // 3. 设计域参数
        out << obj._minMemberStatus <<  obj._minMemberSize
            << obj._maxMemberStatus << obj._maxMemberSize
            << obj._minGapStatus <<  obj._minGapSize
            << obj._stressConstraintStatus << obj._stressConstraintValue;

        // 4. 新增：加强筋参数
        out << obj._ribHeightStatus
            << obj._ribHeight
            << obj._ribWidthStatus
            << obj._ribWidth;

        // 5. 对称约束
        out << obj._patternStatus << obj._patternType << obj._patternNum
            << obj._originId << obj._origin
            << obj._dirPoint1Id << obj._dirPoint1
            << obj._dirPoint2Id << obj._dirPoint2;

        // 4. 拔模约束
        out << obj._liftingStatus << obj._liftingType << obj._nohole
            << obj._startPointId << obj._startPoint
            << obj._dirPointId << obj._dirPoint;

        // 5. 挤出约束
        out << obj._extrusionStatus << obj._extrusionType
            << obj._path1 << obj._path2;

        // 6. 自支撑约束
        out << obj._supportStatus << obj._overhangAngle
            << obj._supportStartPointId << obj._supportStartPoint
            << obj._supportDirPointId << obj._supportDirPoint;

        // 7. 模式重复
        out << obj._patternRepStatus << obj._domainType << obj._coordType
            << obj._repBasePointId << obj._repBasePoint
            << obj._repDirPoint1Id << obj._repDirPoint1
            << obj._repDirPoint2Id << obj._repDirPoint2
            << obj._repDirPoint3Id << obj._repDirPoint3
            << obj._scaleX << obj._scaleY << obj._scaleZ;

        // 8. 形貌参数
        out << obj._morphUpLimit << obj._morphDownLimit << obj._morphInitRatio
            << obj._morphMode << obj._morphMinWidth << obj._morphAngle
            << obj._morphHeight << obj._morphDirection
            << obj._morphBoundStatus
            << obj._morphOriginId << obj._morphOrigin
            << obj._morphDirPoint1Id << obj._morphDirPoint1
            << obj._morphDirPoint2Id << obj._morphDirPoint2;

        // 9. 形状参数
        out << obj._shapeSplitDomain << obj._shapeSplitPart
            << obj._shapeKeepHandle << obj._shape2DSplit
            << obj._shapeDeformMode
            << obj._shapeTx << obj._shapeTy << obj._shapeTz
            << obj._shapeRotAngle
            << obj._shapeRotPick
            << obj._shapeRotAxis << obj._shapeCustomAxis
            << obj._shapeTargetNode
            << obj._shapeInitValue << obj._shapeMinValue
            << obj._shapeMaxValue << obj._shapeMoveLimit;

        return out;
    }

// ==============================================================================
    // mStructOption 核心管理类序列化
    // ==============================================================================
    inline QDataStream& operator<<(QDataStream& out, mStructOption& mgr)
    {
        // 基础数据
        out << mgr._mStructOptionData;

        // ----------------------------------------------------
        // 设计域数据 (Map<int, 指针>)
        out << mgr._maxOptimizationDesignID;
        out << static_cast<qint32>(mgr._designData.size());
        for (auto it = mgr._designData.begin(); it != mgr._designData.end(); ++it)
        {
            out << it->first;           // 存 ID 键
            out << *(it->second);       // 存对象实例
        }

        // ----------------------------------------------------
        // 优化约束数据 (Map<int, 指针>)
        out << mgr._maxOptimizationConstraintID;
        out << static_cast<qint32>(mgr._constraintData.size());
        for (auto it = mgr._constraintData.begin(); it != mgr._constraintData.end(); ++it)
        {
            out << it->first;
            out << *(it->second);
        }

        // ----------------------------------------------------
        // 优化目标数据 (独立指针)
        if (mgr._goalData)
        {
            out << true;                // 标记位：数据存在
            out << *(mgr._goalData);    // 存对象实例
        }
        else
        {
            out << false;               // 标记位：数据为空
        }

        return out;
    }
}