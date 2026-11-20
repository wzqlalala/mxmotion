#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mConnectorManager.h"
#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mConnectorBase.h"
#include "mConnectorBind.h"
#include "mConnectorFlexible.h"
#include "mConnectorMPC.h"
#include "mConnectorRigid.h"
#include "mConnectorDisperse.h"
#include "mConnectingDeputy.h"
#include "mConnectorDistributedCoupling.h"
#include "mConnectorBolt.h"
#include "mConnectorBearingUnits.h"
#include "mConnectorRigidbody.h"

using namespace MxFunctions;
namespace MechConnectorData
{
    inline QDataStream& operator>>(QDataStream& in, mConnectorManager& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammConnectorManagerV3(in, mgr);
        }
        else
        {
            InStreammConnectorManagerV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammConnectorManagerV3(QDataStream& in, mConnectorManager& mgr)
    {
        in >> mgr._maxID;
        int count;
        in >> count;
        for (int i = 0; i < count; ++i) {
            qint32 type;
            in >> type;
            std::shared_ptr<mConnectorBase> obj = nullptr;
            switch ((MxFunctions::ConnectorType)type)
            {
            case MxFunctions::Connector_Rigid:
                obj = std::make_shared<mConnectorRigid>();
                in >> *std::dynamic_pointer_cast<mConnectorRigid>(obj);
                break;
            case MxFunctions::Connector_Flexible:
                obj = std::make_shared<mConnectorFlexible>();
                in >> *std::dynamic_pointer_cast<mConnectorFlexible>(obj);
                break;
            case MxFunctions::Connector_MPC:
                obj = std::make_shared<mConnectorMPC>();
                in >> *std::dynamic_pointer_cast<mConnectorMPC>(obj);
                break;
            case MxFunctions::Connector_Bind:
                obj = std::make_shared<mConnectorBind>();
                in >> *std::dynamic_pointer_cast<mConnectorBind>(obj);
                break;
            case MxFunctions::Connector_Disperse:
                obj = std::make_shared<mConnectorDisperse>();
                in >> *std::dynamic_pointer_cast<mConnectorDisperse>(obj);
                break;
            case MxFunctions::Connector_DistributedCoupling:
                obj = std::make_shared<mConnectorDistributedCoupling>();
                in >> *std::dynamic_pointer_cast<mConnectorDistributedCoupling>(obj);
                break;
            case MxFunctions::Connector_Bolt:
                obj = std::make_shared<mConnectorBolt>();
                in >> *std::dynamic_pointer_cast<mConnectorBolt>(obj);
                break;
            case MxFunctions::Connector_BearingUnits:
                obj = std::make_shared<mConnectorBearingUnits>();
                in >> *std::dynamic_pointer_cast<mConnectorBearingUnits>(obj);
                break;
            case MxFunctions::Connector_Rigidbody:
                obj = std::make_shared<mConnectorRigidbody>();
                in >> *std::dynamic_pointer_cast<mConnectorRigidbody>(obj);
                break;
            default:
                break;
            }

            if (obj)
                mgr.AddConnectorData(obj);
        }
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mConnectorBase& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammConnectorBaseV3(in, obj);
        }
        else
        {
            InStreammConnectorBaseV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammConnectorBaseV3(QDataStream& in, mConnectorBase& obj)
    {
        qint32 type;
        in >> type;
        obj._connectorType = static_cast<MxFunctions::ConnectorType>(type);
        in >> obj._id >> obj._connectorName >> obj._isSuppressed >> obj.userRenamed_;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mConnectorRigid& obj)
    {
        in >> static_cast<mConnectorBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammConnectorRigidV3(in, obj);
        }
        else
        {
            InStreammConnectorRigidV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammConnectorRigidV3(QDataStream& in, mConnectorRigid& obj)
    {
        in >> obj.way_ >> obj.mainnodes_ >> obj.subnodes_ >> obj.degreeoffreedom_main >> obj.tolerance_;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mConnectorFlexible& obj)
    {
        in >> static_cast<mConnectorBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammConnectorFlexibleV3(in, obj);
        }
        else
        {
            InStreammConnectorFlexibleV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammConnectorFlexibleV3(QDataStream& in, mConnectorFlexible& obj)
    {
        in >> obj.way_ >> obj.mainnodes_ >> obj.subnodes_ >> obj.w_ >> obj.dof_;
        in >> obj.singleweight_ >> obj.degreeoffreedom_main >> obj.degreeoffreedom_sub;
        in >> obj.isTableMode_;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mConnectorMPC& obj)
    {
        in >> static_cast<mConnectorBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammConnectorMPCV3(in, obj);
        }
        else
        {
            InStreammConnectorMPCV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammConnectorMPCV3(QDataStream& in, mConnectorMPC& obj)
    {
        in >> obj.subnodes_ >> obj.subDof_ >> obj.subWeights_ >> obj.mainnodes_ >> obj.defaultweights_ >> obj.mainweights_ >> obj.coefficient_;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mConnectorDisperse& obj)
    {
        in >> static_cast<mConnectorBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammConnectorDisperseV3(in, obj);
        }
        else
        {
            InStreammConnectorDisperseV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammConnectorDisperseV3(QDataStream& in, mConnectorDisperse& obj)
    {
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mConnectorDistributedCoupling& obj)
    {
        in >> static_cast<mConnectorBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammConnectorDistributedCouplingV3(in, obj);
        }
        else
        {
            InStreammConnectorDistributedCouplingV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammConnectorDistributedCouplingV3(QDataStream& in, mConnectorDistributedCoupling& obj)
    {
        in >> obj.way_ >> obj.mainnodes_ >> obj.subnodes_ >> obj.w_ >> obj.dof_;
        in >> obj.singleweight_ >> obj.degreeoffreedom_main;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mConnectorBearingUnits& obj)
    {
        in >> static_cast<mConnectorBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammConnectorBearingUnitsV3(in, obj);
        }
        else
        {
            InStreammConnectorBearingUnitsV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammConnectorBearingUnitsV3(QDataStream& in, mConnectorBearingUnits& obj)
    {
        in >> obj.type_ >> obj.node1_ >> obj.node2_;
        in >> obj.rotateFace_ >> obj.defMode_ >> obj.bearingPara_ >> obj.curveRotate_;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mConnectorBind& obj)
    {
        in >> static_cast<mConnectorBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammConnectorBindV3(in, obj);
        }
        else
        {
            InStreammConnectorBindV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammConnectorBindV3(QDataStream& in, mConnectorBind& obj)
    {
        in >> obj.type_ >> obj.allowrancetype_ >> obj.allowrance_ >> obj.adjust_;
        in >> obj._pickStateMaster >> obj._pickStateSlave;
        return in;
    }
  

    inline QDataStream& operator>>(QDataStream& in, mConnectorBolt& obj)
    {
        in >> static_cast<mConnectorBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammConnectorBoltV3(in, obj);
        }
        else
        {
            InStreammConnectorBoltV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammConnectorBoltV3(QDataStream& in, mConnectorBolt& obj)
    {
        in >> obj.type_ >> obj.isOpen_ >> obj.way_ >> obj.dir_;
        in >> obj.x_ >> obj.y_ >> obj.z_ >> obj.node_;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mConnectorRigidbody& obj)
    {
        in >> static_cast<mConnectorBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammConnectorRigidbodyV3(in, obj);
        }
        else
        {
            InStreammConnectorRigidbodyV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammConnectorRigidbodyV3(QDataStream& in, mConnectorRigidbody& obj)
    {
        return in;
    }

}
