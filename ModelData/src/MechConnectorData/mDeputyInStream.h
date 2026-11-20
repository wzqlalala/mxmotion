#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mDeputyManager.h"
#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mDeputyBase.h"

#include "mConnectingDeputy.h"
#include "mConnectorFastener.h"

using namespace MxFunctions;
namespace MechConnectorData
{
    inline QDataStream& operator>>(QDataStream& in, mDeputyManager& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammDeputyManagerV3(in, mgr);
        }
        else
        {
            InStreammDeputyManagerV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammDeputyManagerV3(QDataStream& in, mDeputyManager& mgr)
    {
        in >> mgr._maxID;
        int count;
        in >> count;
        for (int i = 0; i < count; ++i) {
            qint32 type;
            in >> type;
            std::shared_ptr<mDeputyBase> obj = nullptr;
            switch ((MxFunctions::DeputyType)type)
            {
            case MxFunctions::Deputy_Connector:
                obj = std::make_shared<mConnectingDeputy>();
                in >> *std::dynamic_pointer_cast<mConnectingDeputy>(obj);
                break;
            case MxFunctions::Deputy_Fastener:
                obj = std::make_shared<mConnectorFastener>();
                in >> *std::dynamic_pointer_cast<mConnectorFastener>(obj);
                break;
            default:
                break;
            }

            if (obj)
                mgr.AddDeputyData(obj);
        }
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mDeputyBase& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammDeputyBaseV3(in, obj);
        }
        else
        {
            InStreammDeputyBaseV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammDeputyBaseV3(QDataStream& in, mDeputyBase& obj)
    {
        qint32 type;
        in >> type;
        obj._deputyType = static_cast<MxFunctions::DeputyType>(type);
        in >> obj._id >> obj._deputyName >> obj._isSuppressed;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mConnectingDeputy& obj)
    {
        in >> static_cast<mDeputyBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammConnectingDeputyV3(in, obj);
        }
        else
        {
            InStreammConnectingDeputyV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammConnectingDeputyV3(QDataStream& in, mConnectingDeputy& obj)
    {
        in >> obj.refNodeIds_ >> obj.movNodeIds_ >> obj.datatype_ >> obj.subdatatype_;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mConnectorFastener& obj)
    {
        in >> static_cast<mDeputyBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammConnectorFastenerV3(in, obj);
        }
        else
        {
            InStreammConnectorFastenerV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammConnectorFastenerV3(QDataStream& in, mConnectorFastener& obj)
    {
        in >> obj.defineType_ >> obj.nodeId_ >> obj.tempNodeId_ >> obj.influenceRadius_;
        return in;
    }

}