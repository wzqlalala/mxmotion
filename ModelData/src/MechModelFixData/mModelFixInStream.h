#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mModelFixManager.h"
#include "mModelFixDataBase.h"
#include "mEleSetCtrl.h"

namespace MechModelFixData
{
    inline QDataStream& operator>>(QDataStream& in, mModelFixManager& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammModelFixManagerV3(in, mgr);
        }
        else
        {
            InStreammModelFixManagerV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammModelFixManagerV3(QDataStream& in, mModelFixManager& mgr)
    {
        in >> mgr._maxID;
        int count;
        in >> count;
        for (int i = 0; i < count; ++i) {
            QString type;
            in >> type;
            std::shared_ptr<mModelFixDataBase> obj = nullptr;
            if (type == "EleSetCtrl")
            {
                obj = std::make_shared<mEleSetCtrl>();
                in >> *std::dynamic_pointer_cast<mEleSetCtrl>(obj);
            }

            if (obj)
                mgr.AddModelFixData(obj);
        }

       return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mModelFixDataBase& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammModelFixDataBaseV3(in, obj);
        }
        else
        {
            InStreammModelFixDataBaseV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammModelFixDataBaseV3(QDataStream& in, mModelFixDataBase& obj)
    {
        in >> obj._id;
        in >> obj._modelFixName;
        in >> obj._isSuppressed;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mEleSetCtrl& obj)
    {
        in >> static_cast<mModelFixDataBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammEleSetCtrlV3(in, obj);
        }
        else
        {
            InStreammEleSetCtrlV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammEleSetCtrlV3(QDataStream& in, mEleSetCtrl& obj)
    {
        in >> obj._activeOrRemove;
        in >> obj._selectSetType;
        return in;
    }

}
