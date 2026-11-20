#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mFieldManager.h"
#include "mDiscreteField.h"


namespace MechDiscreteFieldData
{
    inline QDataStream& operator>>(QDataStream& in, mFieldManager& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammFieldManagerV3(in, mgr);
        }
        else
        {
            InStreammFieldManagerV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammFieldManagerV3(QDataStream& in, mFieldManager& mgr)
    {
        in >> mgr._maxID;
        int count;
        in >> count;
        for (int i = 0; i < count; ++i) {
            mDiscreteField* obj = new mDiscreteField;
            in >> *obj;

            if (obj)
                mgr.addFieldData(obj);
        }

       return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mDiscreteField& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammDiscreteFieldV3(in, obj);
        }
        else
        {
            InStreammDiscreteFieldV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammDiscreteFieldV3(QDataStream& in, mDiscreteField& obj)
    {
        qint32 locate;
        in >> locate;
        obj._locate = static_cast<MxFunctions::PickDataType>(locate);

        qint32 spaceType;
        in >> spaceType;
        obj._spaceType = static_cast<MxFunctions::DiscreteSpace>(spaceType);

        qint32 coorType;
        in >> coorType;
        obj._coorType = static_cast<MxFunctions::CoordinateType>(coorType);

        in >> obj._id;
        in >> obj._name;
        in >> obj._defaultAngel;
        in >> obj._elementAngelMap;
        in >> obj._defaultThickness;
        in >> obj._elementThicknessMap;
        in >> obj._nodeThicknessMap;
        in >> obj._directPoint1;
        in >> obj._directPoint2;
        in >> obj._directPoint3;
        in >> obj._elementCoorMap;

        return in;
    }

}
