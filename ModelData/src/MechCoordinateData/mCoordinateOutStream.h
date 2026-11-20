#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include <vector>

#include "mCoordinateManager.h"
#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mCoordinateBase.h"

namespace MechCoordinateData
{
    inline QDataStream& operator<<(QDataStream& out, mCoordinateManager& mgr)
    {
        out << mgr._maxID;
        std::vector<int> coorIDs = mgr.getAllCoordinateID(false);
        int count = coorIDs.size();
        out << count;
        for (int i = 0; i < count; ++i) {
            mCoordinateBase* obj = mgr.getCoordinateData(coorIDs[i]);
            out << *obj;
        }
        // 写出节点坐标系数据：节点ID -> <参考坐标系ID, 分析坐标系ID>
        out << mgr._nodeCoordinateData;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, const TransformStep& obj)
    {
        out << static_cast<typename std::underlying_type<MechCoordinateData::TransformType>::type>(obj.type);
        out << obj.value << obj.flipMap << obj.description;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mCoordinateBase& obj)
    {
        qint32 type = static_cast<MxFunctions::CoordinateType>(obj._coordinateType);
        out << type;
        out << obj._id << obj._coordinateName;
        out << obj._origin << obj._mainIndex << obj._dirIndex << obj._xPoint << obj._xyPoint << obj._xReverse << obj._xLine << obj._yLine;
        out << obj._mainBase << obj._mainReverse << obj._dirBase << obj._dirReverse << obj._transformQueue << obj._method;
        out << obj._normaldirection << obj._rotAngleType << obj._rotAngle << obj._rotFieldName;
        return out;
    }

 
}