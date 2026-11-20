#pragma once
//�����������
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mModelEnum.h"
#include "mModelStruct.h"


#include "mLaminateManager.h"
#include "mLaminateBase.h"

#include "mSglLayerPlate.h"
#include "mGlobalSglLam.h"
#include "mLamPlate.h"

namespace MechPropertyData
{
    inline QDataStream& operator>>(QDataStream& in, mLaminateManager& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLaminateManagerV3(in, mgr);
        }
        else
        {
            InStreammLaminateManagerV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammLaminateManagerV3(QDataStream& in, mLaminateManager& mgr)
    {
        in >> mgr._maxID;
        int count;
        in >> count;
        for (int i = 0; i < count; ++i) 
        {
            qint32 type;
            in >> type;
            mLaminateBase* obj = nullptr;
            switch ((MxFunctions::LaminateType)type)
            {
            case MxFunctions::SglLayerPlate:
                obj = new mSglLayerPlate;
                in >> *dynamic_cast<mSglLayerPlate*>(obj);
                break;
            case MxFunctions::GlobalSglLam:
                obj = new mGlobalSglLam;
                in >> *dynamic_cast<mGlobalSglLam*>(obj);
                break;
            case MxFunctions::LamPlate:
                obj = new mLamPlate;
                in >> *dynamic_cast<mLamPlate*>(obj);
                break;
            default:
                break;
            }

            if (obj)
                mgr.AddLaminateData(obj);
        }
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mLaminateBase& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLaminateBaseV3(in, obj);
        }
        else
        {
            InStreammLaminateBaseV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammLaminateBaseV3(QDataStream& in, mLaminateBase& obj)
    {
        qint32 val;
        in >> val;
        obj._LaminateType = static_cast<MxFunctions::LaminateType>(val);
        in >> obj._id >> obj._LaminateName;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mSglLayerPlate& obj)
    {
        in >> static_cast<mLaminateBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammSglLayerPlateV3(in, obj);
        }
        else
        {
            InStreammSglLayerPlateV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammSglLayerPlateV3(QDataStream& in, mSglLayerPlate& obj)
    {
        in >> obj._theta >> obj._thick;
        return in;
    }
   
    
    inline QDataStream& operator>>(QDataStream& in, mGlobalSglLam& obj)
    {
        in >> static_cast<mLaminateBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammGlobalSglLamV3(in, obj);
        }
        else
        {
            InStreammGlobalSglLamV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammGlobalSglLamV3(QDataStream& in, mGlobalSglLam& obj)
    {
        in >> obj._theta >> obj._thick;
        return in;
    }
    
    
    inline QDataStream& operator>>(QDataStream& in, mLamPlate& obj)
    {
        in >> static_cast<mLaminateBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLamPlateV3(in, obj);
        }
        else
        {
            InStreammLamPlateV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammLamPlateV3(QDataStream& in, mLamPlate& obj)
    {
        in >> obj._type;
        return in;
    }

}