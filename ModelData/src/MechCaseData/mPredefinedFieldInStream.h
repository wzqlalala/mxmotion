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
    inline QDataStream& operator>>(QDataStream& in, mPredefinedFieldManager& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammPredefinedFieldManagerV3(in, mgr);
        }
        else
        {
            InStreammPredefinedFieldManagerV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammPredefinedFieldManagerV3(QDataStream& in, mPredefinedFieldManager& mgr)
    {
        in >> mgr._maxID;
        int count;
        in >> count;
        for (int i = 0; i < count; ++i) {
            qint32 type;
            in >> type;
            std::shared_ptr<mPredefinedFieldBase> obj = nullptr;
            switch ((MxFunctions::PredefinedFieldType)type)
            {
            case MxFunctions::PREDEFINED_FIELD_INIT_TEMP:
                obj = std::make_shared<mPredefinedFieldInitTemp>();
                in >> *std::dynamic_pointer_cast<mPredefinedFieldInitTemp>(obj);
                break;
            case MxFunctions::PREDEFINED_FIELD_TEMP_LOAD:
                obj = std::make_shared<mPredefinedFieldTempLoad>();
                in >> *std::dynamic_pointer_cast<mPredefinedFieldTempLoad>(obj);
                break;
            default:
                break;
            }

            if (obj != nullptr)
            {
                mgr.AddPredefinedFieldData(obj);
            }
        }
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mPredefinedFieldBase& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammPredefinedFieldBaseV3(in, obj);
        }
        else
        {
            InStreammPredefinedFieldBaseV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammPredefinedFieldBaseV3(QDataStream& in, mPredefinedFieldBase& obj)
    {
        int id;
        QString name;
        qint32 type;
        in >> id >> name >> type;
        obj.setPredefinedFieldId(id);
        obj.setPredefinedFieldName(name);
        obj.setPredefinedFieldType((MxFunctions::PredefinedFieldType)type);
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mPredefinedFieldInitTemp& obj)
    {
        in >> static_cast<mPredefinedFieldBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammPredefinedFieldInitTempV3(in, obj);
        }
        else
        {
            InStreammPredefinedFieldInitTempV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammPredefinedFieldInitTempV3(QDataStream& in, mPredefinedFieldInitTemp& obj)
    {
        in >> obj._defineType;
        in >> obj._temp;
        in >> obj._nodeTempMap;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mPredefinedFieldTempLoad& obj)
    {
        in >> static_cast<mPredefinedFieldBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammPredefinedFieldTempLoadV3(in, obj);
        }
        else
        {
            InStreammPredefinedFieldTempLoadV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammPredefinedFieldTempLoadV3(QDataStream& in, mPredefinedFieldTempLoad& obj)
    {
        in >> obj._temp;
        in >> obj._defineType;
        in >> obj._nodeTempMap;
        in >> obj._filePath;
        in >> obj._interpolateType;
        in >> obj._midNodeInterpolate;
        return in;
    }

}
