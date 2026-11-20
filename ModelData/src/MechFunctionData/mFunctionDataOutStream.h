#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include <vector>

#include "mFunctionBase.h"
#include "mFunctionManager.h"

namespace MechFunctionData
{
    inline QDataStream& operator<<(QDataStream& out, mFunctionManager& mgr)
    {
        out << mgr._maxID;
        std::vector<int> funcIDs = mgr.getAllFunctionID();
        int count = static_cast<int>(funcIDs.size());
        out << count;
        for (int i = 0; i < count; ++i) {
            mFunctionBase* obj = mgr.getFunctionData(funcIDs[i]);
            out << *obj;
        }
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mFunctionBase& obj)
    {
        out << obj._id << obj._functionName;
        out << obj.FunctionType;
        out << obj._xRangeMin << obj._xRangeMax << obj._varName;
        out << obj.varRangeMin << obj.varRangeMax << obj.expression;
        out << obj._analyticCol;
        out << obj._discreteCol;
        out << obj._interpCol;
        out << obj._piecewiseCol;
        out << obj._stepCol;
        out << obj._tableCol;
        out << obj._periodicCol;
        out << obj._modulatedCol;
        out << obj._expDecayCol;
        return out;
    }
}
