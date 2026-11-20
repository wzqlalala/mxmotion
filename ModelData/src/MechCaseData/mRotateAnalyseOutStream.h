#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mRoatateAnalyseBase.h"
#include "mRotateAnalyseManager.h"
#include "mLoadRotationalSpeed.h"
#include "mLoadRotationalACC.h"

namespace MechCaseData
{
    inline QDataStream& operator<<(QDataStream& out, mRotateAnalyseManager& mgr)
    {
        std::vector<int> ids = mgr.getAllRotateAnalyseID();
        int count = ids.size();
        out << (qint32)count;
        for (int i = 0; i < count; ++i) {
            std::shared_ptr<mRoatateAnalyseBase> obj = mgr.getRotateAnalyseData(ids[i]);
            MxFunctions::RotateAnalysyType type = obj->getRotateAnalyseType();
            out << (qint32)type;
            out << ids[i];
            if (type == MxFunctions::ROTATIONAL_SPEED)
            {
                auto* speed = dynamic_cast<mLoadRotationalSpeed*>(obj.get());
                if (speed)
                    out << *speed;
                else
                    out << *obj;
            }
            else if (type == MxFunctions::ROTATIONAL_ACC)
            {
                auto* acc = dynamic_cast<mLoadRotationalACC*>(obj.get());
                if (acc)
                    out << *acc;
                else
                    out << *obj;
            }
            else
            {
                out << *obj;
            }
        }
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mRoatateAnalyseBase& obj)
    {
        out << obj.getRotateAnalyseName();
        out << (qint32)obj.getRotateAnalyseType();
        return out;
    }
}
