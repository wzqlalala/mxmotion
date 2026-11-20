#pragma once
//�����������
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include <vector>

#include "mContactBase.h"
#include "mContactManager.h"
#include "mModelEnum.h"
#include "mModelStruct.h"

namespace MechContactData
{
    inline QDataStream& operator<<(QDataStream& out, mContactManager& mgr)
    {
        out << mgr._maxID;
        std::vector<int> contactIDs = mgr.getAllContactID();
        int count = contactIDs.size();
        out << count;
        for (int i = 0; i < count; ++i) {
            mContactBase* obj = mgr.getContactData(contactIDs[i]);
            MxFunctions::ContactType type = obj->getContactType();
            out << (qint32)type;
           
            out << *obj;
     
        }
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mContactBase& obj)
    {
        qint32 val = obj._contactType;
        out << val;
        out << obj._id << obj._contactName << obj._contactDescribe << obj._isUserRename
            << obj._isSuppressed
            << obj.masterDir << obj.slaveDir << obj.smallSlip << obj.selfContactSearch
            << obj.overclosure << obj.constraint << obj.scale_factor
            << obj.penalty_factor
            << obj.tangential_define << obj.tangential_friction << obj.tangential_static_coef
            << obj.heat_define << obj.heat_value
            << obj.gap_define << obj.gap_overclosure << obj.geom_define << obj.geom_value
            << obj.clr_fit_typ << obj.size
            << obj._pickStateMaster << obj._pickStateSlave
            << obj.evaluation
            << obj.eva_MP
            << obj.eva_SD
            << obj.eva_CV
            << obj.eva_PC
            << obj.eva_R
            << obj.slave_region;
        return out;
    }
}