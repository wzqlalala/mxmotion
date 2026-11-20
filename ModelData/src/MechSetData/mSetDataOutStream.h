#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include <vector>

#include "mSetBase.h"
#include "mSetDataOutStream.h"
#include "mNode.h"
#include "mSetManager.h"
#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mElementSet.h"
#include "mElementSurface.h"
#include "mGeoSet.h"
#include "mMixSet.h"
#include "mPartSet.h"
#include "mFrequencyPoint.h"
#include "mDOFNode.h"
#include "mPanel.h"
#include "mNodeFace.h"
#include <mSurfaceAnalysis.h>

namespace MechSetData
{
    inline QDataStream& operator<<(QDataStream& out, mSetManager& mgr)
    {
        out << mgr._maxID;
        std::vector<int> setIDs = mgr.getAllSetID(true);
        int count = setIDs.size();
        out << count;
        for (int i = 0; i < count; ++i) {
            mSetBase* obj = mgr.getSetData(setIDs[i]);
            MxFunctions::SetType type = obj->getSetType();
            out << (qint32)type;
            switch (type)
            {
            case MxFunctions::Node:
                out << *dynamic_cast<mNode*>(obj);
                break;
            case MxFunctions::FrequencyPoint:
                out << *dynamic_cast<mFrequencyPoint*>(obj);
                break;
            case MxFunctions::DOFNode:
                out << *dynamic_cast<mDOFNode*>(obj);
                break;
            case MxFunctions::Panel:
                out << *dynamic_cast<mPanel*>(obj);
                break;
            case MxFunctions::Element:
                out << *dynamic_cast<mElementSet*>(obj);
                break;
            case MxFunctions::ElementSurface:
            case MxFunctions::ElementEdge:
                out << *dynamic_cast<mElementSurface*>(obj);
                break;
            case MxFunctions::GeoEdge:
            case MxFunctions::GeoFace:
            case MxFunctions::GeoPoint:
            case MxFunctions::GeoSolid:
                out << *dynamic_cast<mGeoSet*>(obj);
                break;
            case MxFunctions::Part:
                out << *dynamic_cast<mPartSet*>(obj);
                break;
            case MxFunctions::Mix:
                out << *dynamic_cast<mMixSet*>(obj);
                break;
            case MxFunctions::NodeSurface:
                out << *dynamic_cast<mNodeFace*>(obj);
                break;
            case MxFunctions::SurfaceAnalysis:
                out << *dynamic_cast<mSurfaceAnalysis*>(obj);
                break;
            default:
                break;
            }
        }
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mSetBase& obj)
    {
        qint32 val = static_cast<MxFunctions::SetType>(obj._setType);
        out << val;
        out << obj._id << obj._setName << obj._internal << obj._isUserRename;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mNode& obj)
    {
        out << static_cast<mSetBase&>(obj);
        out << obj._isOrder << obj._ids;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mPanel& obj)
    {
        out << static_cast<mSetBase&>(obj);
        out << obj._isOrder << obj._ids << obj._elenames << obj._panelCollectionType << obj._nodeNames;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mFrequencyPoint& obj)
    {
        out << static_cast<mSetBase&>(obj);
        out << obj._order;
        out << obj._isOrder;
        out << obj._ids;
        return out;
    }
    inline QDataStream& operator<<(QDataStream& out, mDOFNode& obj)
    {
        out << static_cast<mSetBase&>(obj);
        out << obj._idof << obj._ids << obj._isOrder;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mElementSet& obj)
    {
        out << static_cast<mSetBase&>(obj);
        out << obj._isOrder << obj._ids;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mElementSurface& obj)
    {
        out << static_cast<mSetBase&>(obj);
        out << obj._ids;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mGeoSet& obj)
    {
        out << static_cast<mSetBase&>(obj);
        out << obj._ids;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mPartSet& obj)
    {
        out << static_cast<mSetBase&>(obj);
        out << obj._ids;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mMixSet& obj)
    {
        out << static_cast<mSetBase&>(obj);
        out << obj._ids;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mNodeFace& obj)
    {
        out << static_cast<mSetBase&>(obj);
        out << obj._ids;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mSurfaceAnalysis& obj)
    {
        out << static_cast<mSetBase&>(obj);
        // 先写子类型
        qint32 val = static_cast<MxFunctions::SetType>(obj._subType);
        out << val;

        switch (obj._subType)
        {
        case RevolutionSurface:
            out << obj._xa << obj._ya << obj._za
                << obj._xb << obj._yb << obj._zb
                << obj._r1 << obj._z1
                << obj._r2 << obj._z2;
            break;

        case CylinderSurface:
            out << obj._ox << obj._oy << obj._oz
                << obj._xx << obj._xy << obj._xz
                << obj._zx << obj._zy << obj._zz
                << obj._sx << obj._sy
                << obj._ex << obj._ey << obj._el;
            break;

        case SegmentSurface:
            out << obj._ox << obj._oy << obj._oz
                << obj._xx << obj._xy << obj._xz
                << obj._zx << obj._zy << obj._zz
                << obj._sx << obj._sy;

            int count = obj._contours.size();
            out << count;
            for (const auto& row : obj._contours)
            {
                out << row.rowType << row.x1 << row.y1 << row.x2 << row.y2;
            }
            break;
        }
        return out;
    }
}