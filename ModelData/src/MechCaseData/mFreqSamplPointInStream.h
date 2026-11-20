#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mFreqSamplPointManager.h"
#include "mFreqSamplPointBase.h"

namespace MechCaseData
{
    inline QDataStream& operator>>(QDataStream& in, mFreqSamplPointManager& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammFreqSamplPointManagerV3(in, mgr);
        }
        else
        {
            InStreammFreqSamplPointManagerV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammFreqSamplPointManagerV3(QDataStream& in, mFreqSamplPointManager& mgr)
    {
        in >> mgr._maxID;
        int count;
        in >> count;
        for (int i = 0; i < count; ++i)
        {
            mFreqSamplPointBase* obj = new mFreqSamplPointBase();
            in >> *(obj);
            mgr.AddFreqSamplPointData(obj);
        }
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mFreqSamplPointBase& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammFreqSamplPointBaseV3(in, obj);
        }
        else
        {
            InStreammFreqSamplPointBaseV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammFreqSamplPointBaseV3(QDataStream& in, mFreqSamplPointBase& obj)
    {
        in >> obj._id >> obj._freqSamplPointName;

        in >> obj._checked >> obj._freqSamplPoint;

        in >> obj._freqSamplPoint1.intervalNum
            >> obj._freqSamplPoint1.startFreq
            >> obj._freqSamplPoint1.stepFreq;

        in >> obj._freqSamplPoint2.startFreq
            >> obj._freqSamplPoint2.stepFreq
            >> obj._freqSamplPoint2.totalPoints;

        in >> obj._freqSamplPoint3.diffType
            >> obj._freqSamplPoint3.endFreq
            >> obj._freqSamplPoint3.expandValue
            >> obj._freqSamplPoint3.refineNum
            >> obj._freqSamplPoint3.startFreq;

        in >> obj._freqSamplPoint4.lowerFreq
            >> obj._freqSamplPoint4.sampleNum
            >> obj._freqSamplPoint4.shiftValue
            >> obj._freqSamplPoint4.stepFreq;

        return in;
    }

}