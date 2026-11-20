#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mFunctionBase.h"
#include "mFunctionManager.h"

namespace MechFunctionData
{
    inline QDataStream& operator>>(QDataStream& in, mFunctionManager& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammFunctionManagerV3(in, mgr);
        }
        else
        {
            InStreammFunctionManagerV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammFunctionManagerV3(QDataStream& in, mFunctionManager& mgr)
    {
        in >> mgr._maxID;
        int count;
        in >> count;
        for (int i = 0; i < count; ++i) {
            mFunctionBase* obj = new mFunctionBase();
            in >> *obj;
            mgr.AddFunctionData(obj);
        }
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mFunctionBase& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammFunctionBaseV3(in, obj);
        }
        else
        {
            InStreammFunctionBaseV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammFunctionBaseV3(QDataStream& in, mFunctionBase& obj)
    {
        in >> obj._id >> obj._functionName;
        in >> obj.FunctionType;
        in >> obj._xRangeMin >> obj._xRangeMax >> obj._varName;
        in >> obj.varRangeMin >> obj.varRangeMax >> obj.expression;
        in >> obj._analyticCol;
        in >> obj._discreteCol;
        in >> obj._interpCol;
        in >> obj._piecewiseCol;
        in >> obj._stepCol;
        in >> obj._tableCol;
        in >> obj._periodicCol;
        in >> obj._modulatedCol;
        in >> obj._expDecayCol;
        return in;
    }

}
