#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include <vector>

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
#include <mConnectorBolt.h>
#include "mConnectorBearingUnits.h"
#include "mConnectorRigidbody.h"

namespace MechConnectorData
{
    inline QDataStream& operator<<(QDataStream& out, mConnectorManager& mgr)
    {
        out << mgr._maxID;
        std::vector<int> ids = mgr.getAllConnectorID();
        int count = ids.size();
        out << count;
        for (int i = 0; i < count; ++i) {
            std::shared_ptr<mConnectorBase> obj = mgr.getConnectorData(ids[i]);
            MxFunctions::ConnectorType type = obj->getConnectorType();
            out << (qint32)type;
            switch (type)
            {
            case MxFunctions::Connector_Rigid:
                out << *std::dynamic_pointer_cast<mConnectorRigid>(obj);
                break;
            case MxFunctions::Connector_Flexible:
                out << *std::dynamic_pointer_cast<mConnectorFlexible>(obj);
                break;
            case MxFunctions::Connector_MPC:
                out << *std::dynamic_pointer_cast<mConnectorMPC>(obj);
                break;
            case MxFunctions::Connector_Bind:
                out << *std::dynamic_pointer_cast<mConnectorBind>(obj);
                break;
            case MxFunctions::Connector_Disperse:
                out << *std::dynamic_pointer_cast<mConnectorDisperse>(obj);
                break;
            case MxFunctions::Connector_DistributedCoupling:
                out << *std::dynamic_pointer_cast<mConnectorDistributedCoupling>(obj);
                break;
            case MxFunctions::Connector_Bolt:
                out << *std::dynamic_pointer_cast<mConnectorBolt>(obj);
                break;
            case MxFunctions::Connector_BearingUnits:
                out << *std::dynamic_pointer_cast<mConnectorBearingUnits>(obj);
                break;
            case MxFunctions::Connector_Rigidbody:
                out << *std::dynamic_pointer_cast<mConnectorRigidbody>(obj);
                break;
            default:
                break;
            }
        }
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mConnectorBase& obj)
    {
        qint32 type = static_cast<MxFunctions::ConnectorType>(obj._connectorType);
        out << type;
        out << obj._id << obj._connectorName << obj._isSuppressed << obj.userRenamed_;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mConnectorRigid& obj)
    {
        out << static_cast<mConnectorBase&>(obj);
        out << obj.way_ << obj.mainnodes_  << obj.subnodes_ << obj.degreeoffreedom_main << obj.tolerance_;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mConnectorFlexible& obj)
    {
        out << static_cast<mConnectorBase&>(obj);
        out << obj.way_ << obj.mainnodes_ << obj.subnodes_ << obj.w_ << obj.dof_;
        out << obj.singleweight_ << obj.degreeoffreedom_main << obj.degreeoffreedom_sub;
        out << obj.isTableMode_;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mConnectorMPC& obj)
    {
        out << static_cast<mConnectorBase&>(obj);
        out << obj.subnodes_ << obj.subDof_ << obj.subWeights_ << obj.mainnodes_ << obj.defaultweights_ << obj.mainweights_ << obj.coefficient_;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mConnectorDisperse& obj)
    {
        out << static_cast<mConnectorBase&>(obj);
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mConnectorDistributedCoupling& obj)
    {
        out << static_cast<mConnectorBase&>(obj);
        out << obj.way_ << obj.mainnodes_ << obj.subnodes_ << obj.w_ << obj.dof_;
        out << obj.singleweight_ << obj.degreeoffreedom_main;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mConnectorBearingUnits& obj)
    {
        out << static_cast<mConnectorBase&>(obj);
        out << obj.type_ << obj.node1_ << obj.node2_;
        out << obj.rotateFace_ << obj.defMode_ << obj.bearingPara_ << obj.curveRotate_;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mConnectorBind& obj)
    {
        out << static_cast<mConnectorBase&>(obj);
        out << obj.type_ << obj.allowrancetype_ << obj.allowrance_ << obj.adjust_ ;
        out << obj._pickStateMaster << obj._pickStateSlave;
        return out;
    }   

    inline QDataStream& operator<<(QDataStream& out, mConnectorBolt& obj)
    {
        out << static_cast<mConnectorBase&>(obj);
        out << obj.type_ << obj.isOpen_ << obj.way_ << obj.dir_;
        out << obj.x_ << obj.y_ << obj.z_ << obj.node_;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mConnectorRigidbody& obj)
    {
        out << static_cast<mConnectorBase&>(obj);
        return out;
    }
}
