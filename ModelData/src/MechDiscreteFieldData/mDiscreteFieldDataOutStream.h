#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include <vector>

#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mFieldManager.h"
#include "mDiscreteField.h"



namespace MechDiscreteFieldData
{
    inline QDataStream& operator<<(QDataStream& out, mFieldManager& mgr)
    {
        out << mgr._maxID;
        std::vector<int> ids = mgr.getAllFieldID(MxFunctions::FieldType::Discrete);
        int count = ids.size();
        out << count;
        for (int i = 0; i < count; ++i) {
            mDiscreteField* obj = mgr.getDiscreteFieldData(ids[i]);
            out << *(obj);
        }
        return out;
    }


    inline QDataStream& operator<<(QDataStream& out, mDiscreteField& obj)
    {
        out << (qint32)obj._locate;
        out << (qint32)obj._spaceType;
        out << (qint32)obj._coorType;

        out << obj._id;
        out << obj._name;
        out << obj._defaultAngel;
        out << obj._elementAngelMap;
        out << obj._defaultThickness;
        out << obj._elementThicknessMap;
        out << obj._nodeThicknessMap;
        out << obj._directPoint1;
        out << obj._directPoint2;
        out << obj._directPoint3;
        out << obj._elementCoorMap;
        return out;
    }


}
