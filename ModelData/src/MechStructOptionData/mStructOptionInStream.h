#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

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
    inline QDataStream& operator>>(QDataStream& in, mStructOptionData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammStructOptionDataV3(in, obj);
        }
        else
        {
            InStreammStructOptionDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammStructOptionDataV3(QDataStream& in, mStructOptionData& obj)
    {
        in >> obj._isMaxiter >> obj._maxiter >> obj._isObjectivetolrance >> obj._objectivetolrance
            >> obj._isDiscretetarget >> obj._discretetarget >> obj._isTopomovelimit >> obj._topomovelimit
            >> obj._isRmin >> obj._rmin >> obj._isPenal >> obj._penal >> obj._isInitdensity >> obj._initdensity
            >> obj._isMinDesity >> obj._minDesity >> obj._isChecker >> obj._checker >> obj._isMatintertype >> obj._matintertype
            >> obj._isRebuild >> obj._rebuild
            >> obj._isRestart >> obj._iteration >> obj._resStartPath;

        return in;
    }



    // ==============================================================================
     // mOptimizationGoalData 序列化
     // ==============================================================================
    inline QDataStream& operator>>(QDataStream& in, mOptimizationGoalData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammOptimizationGoalDataV3(in, obj);
        }
        else
        {
            InStreammOptimizationGoalDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammOptimizationGoalDataV3(QDataStream& in, mOptimizationGoalData& obj)
    {
        in >> obj._objectiveType >> obj._responseType >> obj._frequencyOrder >> obj._isCompliance
            >> obj._isFrequency >> obj._caseToweight >> obj._order >> obj._node
            >> obj._nodeDofs >> obj._frequencyTargetType >> obj._specifiedFrequency >> obj._frfValueType;
        return in;
    }


    // ==============================================================================
    // mOptimizationConstraintData 序列化
    // ==============================================================================

    inline QDataStream& operator>>(QDataStream& in, mOptimizationConstraintData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammOptimizationConstraintDataV3(in, obj);
        }
        else
        {
            InStreammOptimizationConstraintDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammOptimizationConstraintDataV3(QDataStream& in, mOptimizationConstraintData& obj)
    {
        // 基本属性
        in >> obj._id
            >> obj._name
            >> obj._describe;

        // 优化约束
        in >> obj._upperBoundStatus >> obj._upperBoundValue
            >> obj._lowerBoundStatus >> obj._lowerBoundValue
            >> obj._constraintType;

        // 指定区域
        in >> obj._isArea;

        // 响应类型参数(与优化目标一致)
        in >> obj._frequencyOrder >> obj._isCompliance >> obj._isFrequency
            >> obj._caseToweight >> obj._order >> obj._node
            >> obj._nodeDofs >> obj._frequencyTargetType >> obj._specifiedFrequency
            >> obj._frfValueType;

        return in;
    }


    // ==============================================================================
    // mOptimizationDesignData 序列化
    // ==============================================================================

    inline QDataStream& operator>>(QDataStream& in, mOptimizationDesignData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammOptimizationDesignDataV3(in, obj);
        }
        else
        {
            InStreammOptimizationDesignDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammOptimizationDesignDataV3(QDataStream& in, mOptimizationDesignData& obj)
    {
        // 1. 基本属性
        in >> obj._id >> obj._name >> obj._describe;

        // 2. 新增：设计域属性
        in >> obj._designDomainType
            >>  obj._optimizationMethod
            >>  obj._baseThickness;

        // 3. 设计域参数
        in >> obj._minMemberStatus >> obj._minMemberSize
            >> obj._maxMemberStatus >> obj._maxMemberSize
            >> obj._minGapStatus >> obj._minGapSize
            >> obj._stressConstraintStatus >> obj._stressConstraintValue;

        // 4. 新增：加强筋参数
        in >> obj._ribHeightStatus
            >> obj._ribHeight
            >> obj._ribWidthStatus
            >> obj._ribWidth;

        // 5. 对称约束
        in >> obj._patternStatus >> obj._patternType >> obj._patternNum
            >> obj._originId >> obj._origin
            >> obj._dirPoint1Id >> obj._dirPoint1
            >> obj._dirPoint2Id >> obj._dirPoint2;

        // 4. 拔模约束
        in >> obj._liftingStatus >> obj._liftingType >> obj._nohole
            >> obj._startPointId >> obj._startPoint
            >> obj._dirPointId >> obj._dirPoint;

        // 5. 挤出约束
        in >> obj._extrusionStatus >> obj._extrusionType
            >> obj._path1 >> obj._path2;

        // 6. 自支撑约束
        in >> obj._supportStatus >> obj._overhangAngle
            >> obj._supportStartPointId >> obj._supportStartPoint
            >> obj._supportDirPointId >> obj._supportDirPoint;

        // 7. 模式重复
        in >> obj._patternRepStatus >> obj._domainType >> obj._coordType
            >> obj._repBasePointId >> obj._repBasePoint
            >> obj._repDirPoint1Id >> obj._repDirPoint1
            >> obj._repDirPoint2Id >> obj._repDirPoint2
            >> obj._repDirPoint3Id >> obj._repDirPoint3
            >> obj._scaleX >> obj._scaleY >> obj._scaleZ;

        // 8. 形貌参数
        in >> obj._morphUpLimit >> obj._morphDownLimit >> obj._morphInitRatio
            >> obj._morphMode >> obj._morphMinWidth >> obj._morphAngle
            >> obj._morphHeight >> obj._morphDirection
            >> obj._morphBoundStatus
            >> obj._morphOriginId >> obj._morphOrigin
            >> obj._morphDirPoint1Id >> obj._morphDirPoint1
            >> obj._morphDirPoint2Id >> obj._morphDirPoint2;

        // 9. 形状参数
        in >> obj._shapeSplitDomain >> obj._shapeSplitPart
            >> obj._shapeKeepHandle >> obj._shape2DSplit
            >> obj._shapeDeformMode
            >> obj._shapeTx >> obj._shapeTy >> obj._shapeTz
            >> obj._shapeRotAngle
            >> obj._shapeRotPick
            >> obj._shapeRotAxis >> obj._shapeCustomAxis
            >> obj._shapeTargetNode
            >> obj._shapeInitValue >> obj._shapeMinValue
            >> obj._shapeMaxValue >> obj._shapeMoveLimit;

        return in;
    }


    // ==============================================================================
    // mStructOption 核心管理类序列化
    // ==============================================================================

    inline QDataStream& operator>>(QDataStream& in, mStructOption& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammStructOptionV3(in, mgr);
        }
        else
        {
            InStreammStructOptionV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammStructOptionV3(QDataStream& in, mStructOption& mgr)
    {
        // 基础数据
        in >> mgr._mStructOptionData;

        // ----------------------------------------------------
        // 读取设计域数据
        in >> mgr._maxOptimizationDesignID;
        qint32 designSize;
        in >> designSize;
        mgr.deleteAllOptimizationDesignData();
        for (qint32 i = 0; i < designSize; ++i)
        {
            int key;
            in >> key;
            // 动态创建新对象并读入数据
            mOptimizationDesignData* designObj = new mOptimizationDesignData(key, "");
            in >> *designObj;
            mgr._designData[key] = designObj;
        }

        // ----------------------------------------------------
        // 读取优化约束数据
        in >> mgr._maxOptimizationConstraintID;
        qint32 constraintSize;
        in >> constraintSize;
        mgr.deleteAllOptimizationConstraintData();
        for (qint32 i = 0; i < constraintSize; ++i)
        {
            int key;
            in >> key;
            // 动态创建新对象并读入数据
            mOptimizationConstraintData* constraintObj = new mOptimizationConstraintData(key, "");
            in >> *constraintObj;
            mgr._constraintData[key] = constraintObj;
        }

        // ----------------------------------------------------
        // 读取优化目标数据
        bool hasGoalData;
        in >> hasGoalData;
        if (hasGoalData)
        {
            if (!mgr._goalData)
            {
                mgr._goalData = new mOptimizationGoalData();
            }
            in >> *(mgr._goalData);
        }
        else
        {
            // 如果数据源表示无目标数据，重置本地指针
            if (mgr._goalData)
            {
                delete mgr._goalData;
                mgr._goalData = nullptr;
            }
        }

        return in;
    }

}
