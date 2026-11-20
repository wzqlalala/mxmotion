#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include <vector>

#include "mDeputyManager.h"
#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mDeputyBase.h"

#include "mConnectingDeputy.h"
#include "mConnectorFastener.h"

namespace MechConnectorData
{
    inline QDataStream& operator<<(QDataStream& out, mDeputyManager& mgr)
    {
        out << mgr._maxID;
        std::vector<int> ids = mgr.getAllDeputyID();
        int count = ids.size();
        out << count;
        for (int i = 0; i < count; ++i) {
            std::shared_ptr<mDeputyBase> obj = mgr.getDeputyData(ids[i]);
            MxFunctions::DeputyType type = obj->getDeputyType();
            out << (qint32)type;
            switch (type)
            {
            case MxFunctions::Deputy_Connector:
                out << *std::dynamic_pointer_cast<mConnectingDeputy>(obj);
                break;
            case MxFunctions::Deputy_Fastener:
                out << *std::dynamic_pointer_cast<mConnectorFastener>(obj);
                break;
            default:
                break;
            }
        }
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mDeputyBase& obj)
    {
        qint32 type = static_cast<MxFunctions::DeputyType>(obj._deputyType);
        out << type;
        out << obj._id << obj._deputyName << obj._isSuppressed;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mConnectingDeputy& obj)
    {
        out << static_cast<mDeputyBase&>(obj);
        out << obj.refNodeIds_ << obj.movNodeIds_ << obj.datatype_ << obj.subdatatype_;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mConnectorFastener& obj)
    {
        out << static_cast<mDeputyBase&>(obj);
        out << obj.defineType_ << obj.nodeId_ << obj.tempNodeId_ << obj.influenceRadius_;
        return out;
    }
}