#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include <vector>

#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mRunnerStructOption.h"
#include "mRunnerOptimizationDesignData.h"
#include "mRunnerOptimizationConstraintData.h"
#include "mRunnerOptimizationGoalData.h"


namespace MechStructOptionData
{
    // ==============================================================================
     // mRunnerOptionData 序列化
     // ==============================================================================
    inline QDataStream& operator<<(QDataStream& out, mRunnerOptionData& obj)
    {
        out << obj._isMaxiter << obj._maxiter << obj._isObjectivetolrance << obj._objectivetolrance
            << obj._isDiscretetarget << obj._discretetarget << obj._isTopomovelimit << obj._topomovelimit
            << obj._isRmin << obj._rmin << obj._isPenal << obj._penal
            << obj._density << obj._dynamicViscosity;
       return out;
    }

    // ==============================================================================
    // mRunnerOptimizationGoalData 序列化
    // ==============================================================================
    inline QDataStream& operator<<(QDataStream& out, mRunnerOptimizationGoalData& obj)
    {
        out << obj._objectiveType << obj._responseType;
        return out;
    }

    // ==============================================================================
    // mRunnerOptimizationDesignData 序列化
    // ==============================================================================
    inline QDataStream& operator<<(QDataStream& out, mRunnerOptimizationDesignData& obj)
    {
        // 1. 基本属性
        out << obj._id << obj._name << obj._describe;

        // 2. 设计域属性
        out << obj._designDomainType;

        // 3. 达西惩罚项
        out << obj._maxPermeabilityStatus << obj._maxPermeability
            << obj._minPermeabilityStatus << obj._minPermeability;

        // 4. 设计域参数
        out << obj._minMemberStatus << obj._minMemberSize
            << obj._maxMemberStatus << obj._maxMemberSize
            << obj._minGapStatus << obj._minGapSize;

        // 5. 对称约束
        out << obj._patternStatus << obj._patternType << obj._patternNum
            << obj._originId << obj._origin
            << obj._dirPoint1Id << obj._dirPoint1
            << obj._dirPoint2Id << obj._dirPoint2;

        // 6. 拔模约束
        out << obj._liftingStatus << obj._liftingType << obj._nohole
            << obj._startPointId << obj._startPoint
            << obj._dirPointId << obj._dirPoint;

        // 7. 挤出约束
        out << obj._extrusionStatus << obj._extrusionType
            << obj._path1 << obj._path2;

        // 8. 自支撑约束
        out << obj._supportStatus << obj._overhangAngle
            << obj._supportStartPointId << obj._supportStartPoint
            << obj._supportDirPointId << obj._supportDirPoint;

        // 9. 模式重复
        out << obj._patternRepStatus << obj._domainType << obj._coordType
            << obj._repBasePointId << obj._repBasePoint
            << obj._repDirPoint1Id << obj._repDirPoint1
            << obj._repDirPoint2Id << obj._repDirPoint2
            << obj._repDirPoint3Id << obj._repDirPoint3
            << obj._scaleX << obj._scaleY << obj._scaleZ;

        return out;
    }

    // ==============================================================================
    // mRunnerOptimizationConstraintData 序列化
    // ==============================================================================
    inline QDataStream& operator<<(QDataStream& out, mRunnerOptimizationConstraintData& obj)
    {
        // 基本属性
        out << obj._id
            << obj._name
            << obj._describe;

        // 优化约束
        out << obj._upperBoundStatus << obj._upperBoundValue
            << obj._lowerBoundStatus << obj._lowerBoundValue
            << obj._constraintType
            << obj._constraintMode;

        // 指定区域
        out << obj._isArea;

        return out;
    }

// ==============================================================================
    // mRunnerStructOption 核心管理类序列化
    // ==============================================================================
    inline QDataStream& operator<<(QDataStream& out, mRunnerStructOption& mgr)
    {
        // 基础数据
        out << mgr._mRunnerOptionData;

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
