#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mRoatateAnalyseBase.h"
#include "mLoadRotationalACC.h"
#include "mLoadRotationalSpeed.h"
#include "mRotateAnalyseManager.h"

namespace MechCaseData
{
    inline QDataStream& operator>>(QDataStream& in, mRotateAnalyseManager& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammRotateAnalyseManagerV3(in, mgr);
        }
        else
        {
            InStreammRotateAnalyseManagerV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammRotateAnalyseManagerV3(QDataStream& in, mRotateAnalyseManager& mgr)
    {
        int count = 0;
        in >> count;
        for (int i = 0; i < count; ++i) {
            qint32 type;
            in >> type;
            int id = 0;
            in >> id;
            std::shared_ptr<mRoatateAnalyseBase> obj = nullptr;
            switch ((MxFunctions::RotateAnalysyType)type)
            {
            case MxFunctions::ROTATIONAL_ACC:
                obj = std::make_shared<mLoadRotationalACC>();
                in >> *dynamic_cast<mLoadRotationalACC*>(obj.get());
                break;
            case MxFunctions::ROTATIONAL_SPEED:
                obj = std::make_shared<mLoadRotationalSpeed>();
                in >> *dynamic_cast<mLoadRotationalSpeed*>(obj.get());
                break;
            default:
                obj = std::make_shared<mLoadRotationalACC>();
                in >> *obj;
                break;
            }
            obj->setRotateAnalyseId(id);
            mgr.AddRotateAnalyseData(obj);
            //文件中没有保存 _maxID，读完后按已加载的最大 ID 恢复，避免新建数据时 ID 重复
            if (id > mgr._maxID)
                mgr._maxID = id;
        }
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mRoatateAnalyseBase& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammRoatateAnalyseBaseV3(in, obj);
        }
        else
        {
            InStreammRoatateAnalyseBaseV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammRoatateAnalyseBaseV3(QDataStream& in, mRoatateAnalyseBase& obj)
    {
        QString name;
        qint32 type;
        in >> name;
        in >> type;
        obj.setRotateAnalyseName(name);
        obj.setRotateAnalyseType((MxFunctions::RotateAnalysyType)type);
        return in;
    }

}
