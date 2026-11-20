#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include <vector>

#include "mMaterialBase.h"
#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mMaterialManager.h"

namespace MechMaterialData
{
    inline QDataStream& operator<<(QDataStream& out, mMaterialManager& mgr)
    {
        out << mgr._maxID;
        std::vector<int> matIDs = mgr.getAllMatID();
        int count = matIDs.size();
        out << count;
        for (int i = 0; i < count; ++i) {
            std::shared_ptr<mMaterialBase> obj = mgr.getMaterialData(matIDs[i]);
            out << *(obj);
        }
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mMaterialBase& obj)
    {
        qint32 val = static_cast<MxFunctions::MaterialClass>(obj._materialType);
        out << val;
        out << obj._id << obj._materialName << obj._color;
        int count = obj._props.size();
        out << count;
        for (auto it = obj._props.begin(); it != obj._props.end(); ++it)
        {
            val = static_cast<MxFunctions::MaterialClass>(it.key());
            out << val;
            count = it.value().size();
            out << count;
            for (auto itt = it.value().begin(); itt != it.value().end(); ++itt)
            {
                val = static_cast<MxFunctions::MaterialPropType>(itt.key());
                out << val;
                out << itt.value();
            }
        }

        count = obj._libprops.size();
        out << count;
        for (auto it = obj._libprops.begin(); it != obj._libprops.end(); ++it)
        {
            val = static_cast<MxFunctions::MaterialClass>(it.key());
            out << val;
            count = it.value().size();
            out << count;
            for (auto itt = it.value().begin(); itt != it.value().end(); ++itt)
            {
                out << itt.key();
                QVector<Unit>& unit = itt.value().first;
                count = unit.size();
                out << count;
                for (int i = 0; i < count; i++)
                {
                    out << static_cast<typename std::underlying_type<Unit>::type>(unit[i]);
                }
                out << itt.value().second;
            }
        }

        return out;
    }


}