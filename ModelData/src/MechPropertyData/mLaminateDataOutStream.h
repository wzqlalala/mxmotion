#pragma once
//解决中文乱码
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
    inline QDataStream& operator<<(QDataStream& out, mLaminateManager& mgr)
    {
        out << mgr._maxID;
        std::vector<int> laminateIDs = mgr.getAllLaminateID();
        int count = laminateIDs.size();
        out << count;
        for (int i = 0; i < count; ++i) 
        {
            mLaminateBase* obj = mgr.getLaminateData(laminateIDs[i]);
            MxFunctions::LaminateType type = obj->getLaminateType();
            out << (qint32)type;
            switch (type)
            {
            case MxFunctions::SglLayerPlate:
                out << *dynamic_cast<mSglLayerPlate*>(obj);
                break;
            case MxFunctions::GlobalSglLam:
                out << *dynamic_cast<mGlobalSglLam*>(obj);
                break;
            case MxFunctions::LamPlate:
                out << *dynamic_cast<mLamPlate*>(obj);
                break;
            default:
                break;
            }
        }
        return out;
    }


    inline QDataStream& operator<<(QDataStream& out, mLaminateBase& obj)
    {
        qint32 val = static_cast<MxFunctions::LaminateType>(obj._LaminateType);
        out << val;
        out << obj._id << obj._LaminateName;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mSglLayerPlate& obj)
    {
        out << static_cast<mLaminateBase&>(obj);
        out << obj._theta << obj._thick;
        return out;
    }    
    
    inline QDataStream& operator<<(QDataStream& out, mGlobalSglLam& obj)
    {
        out << static_cast<mLaminateBase&>(obj);
        out << obj._theta << obj._thick;
        return out;
    }    
    
    inline QDataStream& operator<<(QDataStream& out, mLamPlate& obj)
    {
        out << static_cast<mLaminateBase&>(obj);
        out << obj._type;
        return out;
    }
}
