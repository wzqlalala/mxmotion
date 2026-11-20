#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mFreqSamplPointManager.h"
#include "mFreqSamplPointBase.h"
namespace MechCaseData
{
    inline QDataStream& operator<<(QDataStream& out, mFreqSamplPointManager& mgr)
    {
        out << mgr._maxID;
        std::vector<int> IDs = mgr.getAllFreqSamplPointID();
        int count = IDs.size();
        out << count;
        for (int i = 0; i < count; ++i)
        {
            mFreqSamplPointBase* obj = mgr.getFreqSamplPointData(IDs[i]);
            out << *(obj);
        }
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mFreqSamplPointBase& obj)
    {
        out << obj._id << obj._freqSamplPointName;

        out << obj._checked << obj._freqSamplPoint;

        out << obj._freqSamplPoint1.intervalNum
            << obj._freqSamplPoint1.startFreq
            << obj._freqSamplPoint1.stepFreq;

        out << obj._freqSamplPoint2.startFreq
            << obj._freqSamplPoint2.stepFreq
            << obj._freqSamplPoint2.totalPoints;

        out << obj._freqSamplPoint3.diffType
            << obj._freqSamplPoint3.endFreq
            << obj._freqSamplPoint3.expandValue
            << obj._freqSamplPoint3.refineNum
            << obj._freqSamplPoint3.startFreq;

        out << obj._freqSamplPoint4.lowerFreq
            << obj._freqSamplPoint4.sampleNum
            << obj._freqSamplPoint4.shiftValue
            << obj._freqSamplPoint4.stepFreq;

        return out;
    }
}