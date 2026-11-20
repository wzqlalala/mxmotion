#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mModelEnum.h"
#include "mModelStruct.h"

#include "mPredefinedFieldManager.h"
#include "mPredefinedFieldBase.h"

#include "mPredefinedFieldInitTemp.h"
#include "mPredefinedFieldTempLoad.h"

namespace MechCaseData
{
    inline QDataStream& operator<<(QDataStream& out, mPredefinedFieldManager& mgr)
    {
        out << mgr._maxID;
        std::vector<int> fieldIDs = mgr.getAllPredefinedFieldID();
        int count = fieldIDs.size();
        out << count;
        for (int i = 0; i < count; ++i) {
            std::shared_ptr<mPredefinedFieldBase> obj = mgr.getPredefinedFieldData(fieldIDs[i]);
            MxFunctions::PredefinedFieldType type = obj->getPredefinedFieldType();
            out << (qint32)type;
            switch (type)
            {
            case MxFunctions::PREDEFINED_FIELD_INIT_TEMP:
                out << *std::dynamic_pointer_cast<mPredefinedFieldInitTemp>(obj);
                break;
            case MxFunctions::PREDEFINED_FIELD_TEMP_LOAD:
                out << *std::dynamic_pointer_cast<mPredefinedFieldTempLoad>(obj);
                break;
            default:
                break;
            }
        }
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mPredefinedFieldBase& obj)
    {
        out << obj.getPredefinedFieldId();
        out << obj.getPredefinedFieldName();
        out << (qint32)obj.getPredefinedFieldType();
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mPredefinedFieldInitTemp& obj)
    {
        out << static_cast<mPredefinedFieldBase&>(obj);
        out << obj._defineType;
        out << obj._temp;
        out << obj._nodeTempMap;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mPredefinedFieldTempLoad& obj)
    {
        out << static_cast<mPredefinedFieldBase&>(obj);
        out << obj._temp;
        out << obj._defineType;
        out << obj._nodeTempMap;
        out << obj._filePath;
        out << obj._interpolateType;
        out << obj._midNodeInterpolate;
        return out;
    }
}
