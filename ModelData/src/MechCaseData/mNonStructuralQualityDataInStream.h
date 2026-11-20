#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mNonStructQualityManager.h"
#include "mNonStructuralQuality.h"


namespace MechCaseData
{
    inline QDataStream& operator>>(QDataStream& in, mNonStructQualityManager& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammNonStructQualityManagerV3(in, mgr);
        }
        else
        {
            InStreammNonStructQualityManagerV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammNonStructQualityManagerV3(QDataStream& in, mNonStructQualityManager& mgr)
    {
        in >> mgr._maxID;
        int count;
        in >> count;
        for (int i = 0; i < count; ++i) {
            mNonStructuralQuality* obj = new mNonStructuralQuality;
            in >> *obj;
            if (obj)
                mgr.AddNsmData(obj);
        }

       return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mNonStructuralQuality& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammNonStructuralQualityV3(in, obj);
        }
        else
        {
            InStreammNonStructuralQualityV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammNonStructuralQualityV3(QDataStream& in, mNonStructuralQuality& obj)
    {
        /*qint32 val;
        in >> val;
        obj._propertyType = static_cast<MxFunctions::PropertyType>(val);*/

        qint32 typeIndex;
        in >> typeIndex;
        obj._typeIndex = static_cast<MxFunctions::Non>(typeIndex);

        qint32 propIndex;
        in >> propIndex;
        obj._propIndex = static_cast<MxFunctions::PhysicalFieldType>(propIndex);

        in >> obj._id;

        in  >> obj._unionValue >> obj._nsmName
            >> obj._crossPropertyData >> obj._elementData >> obj._elementSetData;
        return in;
    }


    
}
