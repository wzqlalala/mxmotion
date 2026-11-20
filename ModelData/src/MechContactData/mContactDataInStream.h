#pragma once
//�����������
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mContactManager.h"
#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mContactBase.h"

using namespace MxFunctions;
namespace MechContactData
{
    inline QDataStream& operator>>(QDataStream& in, mContactManager& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammContactManagerV3(in, mgr);
        }
        else
        {
            InStreammContactManagerV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammContactManagerV3(QDataStream& in, mContactManager& mgr)
    {
        in >> mgr._maxID;
        int count;
        in >> count;
        for (int i = 0; i < count; ++i) {
            qint32 type;
            in >> type;
            mContactBase* obj = new mContactBase(0, ContactType(type));
            in >> *obj;

            if (obj)
                mgr.AddContactData(obj);
        }
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mContactBase& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammContactBaseV3(in, obj);
        }
        else
        {
            InStreammContactBaseV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammContactBaseV3(QDataStream& in, mContactBase& obj)
    {
        qint32 val;
        in >> val;
        obj._contactType = static_cast<ContactType>(val);
        in >> obj._id >> obj._contactName >> obj._contactDescribe >> obj._isUserRename
            >> obj._isSuppressed
            >> obj.masterDir >> obj.slaveDir >> obj.smallSlip >> obj.selfContactSearch
            >> obj.overclosure >> obj.constraint >> obj.scale_factor
            >> obj.penalty_factor
            >> obj.tangential_define >> obj.tangential_friction >> obj.tangential_static_coef
            >> obj.heat_define >> obj.heat_value
            >> obj.gap_define >> obj.gap_overclosure >> obj.geom_define >> obj.geom_value
            >> obj.clr_fit_typ >> obj.size
            >> obj._pickStateMaster >> obj._pickStateSlave
            >> obj.evaluation
            >> obj.eva_MP
            >> obj.eva_SD
            >> obj.eva_CV
            >> obj.eva_PC
            >> obj.eva_R;
        //0x3001 追加：从面区域类型（旧版本文件无该字段，保持默认值 0=单元面）
        if (FILE_VERSION >= 0x3001)
            in >> obj.slave_region;
        return in;
    }

}
