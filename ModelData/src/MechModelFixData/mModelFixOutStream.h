#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include <vector>

#include "mModelFixManager.h"
#include "mModelFixDataBase.h"
#include "mEleSetCtrl.h"

namespace MechModelFixData
{
    inline QDataStream& operator<<(QDataStream& out, mModelFixManager& mgr)
    {
        out << mgr._maxID;
        std::vector<int> ids = mgr.getAllModelFixID();
        int count = ids.size();
        out << count;
        for (int i = 0; i < count; ++i) {
            std::shared_ptr<mModelFixDataBase> obj = mgr.getModelFixData(ids[i]);
            if (obj == nullptr)
                continue;
            out << obj->getModelFixType();
            QString type = obj->getModelFixType();
            if (type == "EleSetCtrl")
            {
                out << *std::dynamic_pointer_cast<mEleSetCtrl>(obj);
            }
        }
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mModelFixDataBase& obj)
    {
        out << obj._id;
        out << obj._modelFixName;
        out << obj._isSuppressed;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mEleSetCtrl& obj)
    {
        out << static_cast<mModelFixDataBase&>(obj);
        out << obj._activeOrRemove;
        out << obj._selectSetType;
        return out;
    }
}
