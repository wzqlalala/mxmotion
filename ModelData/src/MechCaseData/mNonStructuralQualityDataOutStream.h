#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include <vector>

#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mNonStructQualityManager.h"
#include "mNonStructuralQuality.h"



namespace MechCaseData
{
    inline QDataStream& operator<<(QDataStream& out, mNonStructQualityManager& mgr)
    {
        out << mgr._maxID;
        std::vector<int> nonIds = mgr.getAllNonSturcturalQualityID();
        int count = nonIds.size();
        out << count;
        for (int i = 0; i < count; ++i) {
            mNonStructuralQuality* obj = mgr.getNonStructuralQualityData(nonIds[i]);
            out << *(obj);
        }
        return out;
    }


    inline QDataStream& operator<<(QDataStream& out, mNonStructuralQuality& obj)
    {
        out << (qint32)obj._typeIndex;
        out << (qint32)obj._propIndex;

        out << obj._id;

        out << obj._unionValue << obj._nsmName
            << obj._crossPropertyData << obj._elementData << obj._elementSetData;
        return out;
    }

   
}
