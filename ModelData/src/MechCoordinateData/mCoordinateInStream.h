#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mCoordinateManager.h"
#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mCoordinateBase.h"

using namespace MxFunctions;
namespace MechCoordinateData
{
    inline QDataStream& operator>>(QDataStream& in, mCoordinateManager& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammCoordinateManagerV3(in, mgr);
        }
        else
        {
            InStreammCoordinateManagerV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammCoordinateManagerV3(QDataStream& in, mCoordinateManager& mgr)
    {
        in >> mgr._maxID;
        int count;
        in >> count;
        for (int i = 0; i < count; ++i) {
            mCoordinateBase* obj = new mCoordinateBase;
            in >> *obj;
            if (obj)
                mgr.AddCoordinateData(obj);
        }
        // 读取节点坐标系数据：节点ID -> <参考坐标系ID, 分析坐标系ID>
        in >> mgr._nodeCoordinateData;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, TransformStep& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamTransformStepV3(in, obj);
        }
        else
        {
            InStreamTransformStepV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreamTransformStepV3(QDataStream& in, TransformStep& obj)
    {
        typename std::underlying_type<TransformType>::type rawKey;
        in >> rawKey;
        obj.type = static_cast<TransformType>(rawKey);

        in >> obj.value >> obj.flipMap >> obj.description;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mCoordinateBase& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammCoordinateBaseV3(in, obj);
        }
        else
        {
            InStreammCoordinateBaseV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammCoordinateBaseV3(QDataStream& in, mCoordinateBase& obj)
    {
        qint32 type;
        in >> type;
        obj._coordinateType = static_cast<MxFunctions::CoordinateType>(type);
        in >> obj._id >> obj._coordinateName;
        in >> obj._origin >> obj._mainIndex >> obj._dirIndex >> obj._xPoint >> obj._xyPoint >> obj._xReverse >> obj._xLine >> obj._yLine;
        in >> obj._mainBase >> obj._mainReverse >> obj._dirBase >> obj._dirReverse >> obj._transformQueue >> obj._method;
        in >> obj._normaldirection >> obj._rotAngleType >> obj._rotAngle >> obj._rotFieldName;
        return in;
    }




}
