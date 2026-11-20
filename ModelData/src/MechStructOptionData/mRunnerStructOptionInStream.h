#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mRunnerStructOption.h"
#include "mRunnerOptimizationDesignData.h"
#include "mRunnerOptimizationConstraintData.h"
#include "mRunnerOptimizationGoalData.h"

namespace MechStructOptionData
{
    // ==============================================================================
     // mRunnerOptionData 反序列化
     // ==============================================================================
    inline QDataStream& operator>>(QDataStream& in, mRunnerOptionData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammRunnerOptionDataV3(in, obj);
        }
        else
        {
            InStreammRunnerOptionDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammRunnerOptionDataV3(QDataStream& in, mRunnerOptionData& obj)
    {
        in >> obj._isMaxiter >> obj._maxiter >> obj._isObjectivetolrance >> obj._objectivetolrance
            >> obj._isDiscretetarget >> obj._discretetarget >> obj._isTopomovelimit >> obj._topomovelimit
            >> obj._isRmin >> obj._rmin >> obj._isPenal >> obj._penal
            >> obj._density >> obj._dynamicViscosity;
        return in;
    }


    // ==============================================================================
    // mRunnerOptimizationGoalData 反序列化
    // ==============================================================================
    inline QDataStream& operator>>(QDataStream& in, mRunnerOptimizationGoalData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammRunnerOptimizationGoalDataV3(in, obj);
        }
        else
        {
            InStreammRunnerOptimizationGoalDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammRunnerOptimizationGoalDataV3(QDataStream& in, mRunnerOptimizationGoalData& obj)
    {
        in >> obj._objectiveType >> obj._responseType;
        return in;
    }


    // ==============================================================================
    // mRunnerOptimizationDesignData 反序列化
    // ==============================================================================

    inline QDataStream& operator>>(QDataStream& in, mRunnerOptimizationDesignData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammRunnerOptimizationDesignDataV3(in, obj);
        }
        else
        {
            InStreammRunnerOptimizationDesignDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammRunnerOptimizationDesignDataV3(QDataStream& in, mRunnerOptimizationDesignData& obj)
    {
        // 1. 基本属性
        in >> obj._id >> obj._name >> obj._describe;

        // 2. 设计域属性
        in >> obj._designDomainType;

        // 3. 达西惩罚项
        in >> obj._maxPermeabilityStatus >> obj._maxPermeability
            >> obj._minPermeabilityStatus >> obj._minPermeability;

        // 4. 设计域参数
        in >> obj._minMemberStatus >> obj._minMemberSize
            >> obj._maxMemberStatus >> obj._maxMemberSize
            >> obj._minGapStatus >> obj._minGapSize;

        // 5. 对称约束
        in >> obj._patternStatus >> obj._patternType >> obj._patternNum
            >> obj._originId >> obj._origin
            >> obj._dirPoint1Id >> obj._dirPoint1
            >> obj._dirPoint2Id >> obj._dirPoint2;

        // 6. 拔模约束
        in >> obj._liftingStatus >> obj._liftingType >> obj._nohole
            >> obj._startPointId >> obj._startPoint
            >> obj._dirPointId >> obj._dirPoint;

        // 7. 挤出约束
        in >> obj._extrusionStatus >> obj._extrusionType
            >> obj._path1 >> obj._path2;

        // 8. 自支撑约束
        in >> obj._supportStatus >> obj._overhangAngle
            >> obj._supportStartPointId >> obj._supportStartPoint
            >> obj._supportDirPointId >> obj._supportDirPoint;

        // 9. 模式重复
        in >> obj._patternRepStatus >> obj._domainType >> obj._coordType
            >> obj._repBasePointId >> obj._repBasePoint
            >> obj._repDirPoint1Id >> obj._repDirPoint1
            >> obj._repDirPoint2Id >> obj._repDirPoint2
            >> obj._repDirPoint3Id >> obj._repDirPoint3
            >> obj._scaleX >> obj._scaleY >> obj._scaleZ;

        return in;
    }


    // ==============================================================================
    // mRunnerOptimizationConstraintData 反序列化
    // ==============================================================================

    inline QDataStream& operator>>(QDataStream& in, mRunnerOptimizationConstraintData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammRunnerOptimizationConstraintDataV3(in, obj);
        }
        else
        {
            InStreammRunnerOptimizationConstraintDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammRunnerOptimizationConstraintDataV3(QDataStream& in, mRunnerOptimizationConstraintData& obj)
    {
        // 基本属性
        in >> obj._id
            >> obj._name
            >> obj._describe;

        // 优化约束
        in >> obj._upperBoundStatus >> obj._upperBoundValue
            >> obj._lowerBoundStatus >> obj._lowerBoundValue
            >> obj._constraintType
            >> obj._constraintMode;

        // 指定区域
        in >> obj._isArea;

        return in;
    }


    // ==============================================================================
    // mRunnerStructOption 核心管理类反序列化
    // ==============================================================================

    inline QDataStream& operator>>(QDataStream& in, mRunnerStructOption& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammRunnerStructOptionV3(in, mgr);
        }
        else
        {
            InStreammRunnerStructOptionV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammRunnerStructOptionV3(QDataStream& in, mRunnerStructOption& mgr)
    {
        // 基础数据
        in >> mgr._mRunnerOptionData;

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
            mRunnerOptimizationDesignData* designObj = new mRunnerOptimizationDesignData(key, "");
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
            mRunnerOptimizationConstraintData* constraintObj = new mRunnerOptimizationConstraintData(key, "");
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
                mgr._goalData = new mRunnerOptimizationGoalData();
            }
            in >> *(mgr._goalData);
        }
        else
        {
            if (mgr._goalData)
            {
                delete mgr._goalData;
                mgr._goalData = nullptr;
            }
        }

        return in;
    }

}
