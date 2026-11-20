#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mMaterialBase.h"
#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mMaterialManager.h"

namespace MechMaterialData
{
    inline QDataStream& operator>>(QDataStream& in, mMaterialManager& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammMaterialManagerV3(in, mgr);
        }
        else
        {
            InStreammMaterialManagerV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammMaterialManagerV3(QDataStream& in, mMaterialManager& mgr)
    {
        in >> mgr._maxID;
        int count;
        in >> count;
        for (int i = 0; i < count; ++i) {
            std::shared_ptr<mMaterialBase> obj = std::make_shared<mMaterialBase>();
            in >> *(obj);
            mgr.AddMaterialData(obj);
        }
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mMaterialBase& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammMaterialBaseV3(in, obj);
        }
        else
        {
            InStreammMaterialBaseV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammMaterialBaseV3(QDataStream& in, mMaterialBase& obj)
    {
        InStreamMatBaseV3(in, obj);
        return in;
    }


    inline QDataStream& InStreamMatBaseV3(QDataStream& in, mMaterialBase& obj)
    {
        qint32 val;
        in >> val;
        obj._materialType = static_cast<MxFunctions::MaterialType>(val);
        in >> obj._id >> obj._materialName >> obj._color;
        int count;
        in >> count;
        for (int i = 0; i < count; i++)
        {
            in >> val;
            MaterialClass _propsType;
            _propsType = static_cast<MxFunctions::MaterialClass>(val);

            int _propsCount;
            in >> _propsCount;
            for (int j = 0; j < _propsCount; j++)
            {
                in >> val;
                MaterialPropType propType = static_cast<MxFunctions::MaterialPropType>(val);
                QVariant var;
                in >> var;
                obj._props[_propsType][propType] = var;
            }
        }

        in >> count;
        for (int i = 0; i < count; i++)
        {
            in >> val;
            MaterialClass _propsType;
            _propsType = static_cast<MxFunctions::MaterialClass>(val);

            int _propsCount;
            in >> _propsCount;
            for (int j = 0; j < _propsCount; j++)
            {
                QString str;
                in >> str;

                QVector<Unit> unit;
                int unitCount;
                in >> unitCount;
                for (int k = 0; k < unitCount; k++)
                {
                    typename std::underlying_type<MxFunctions::Unit>::type rawKey;
                    in >> rawKey;
                    unit.push_back(static_cast<MxFunctions::Unit>(rawKey));
                }
                QVariant var;
                in >> var;
                obj._libprops[_propsType][str] = { unit , var };
            }
        }
        
        return in;
    }


}
