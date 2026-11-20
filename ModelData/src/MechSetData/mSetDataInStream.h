#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mNode.h"
#include "mSetManager.h"
#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mSetBase.h"
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
    inline QDataStream& operator>>(QDataStream& in, mSetManager& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamSetManagerV3(in, mgr);
        }
        else
        {
            InStreamSetManagerV3(in, mgr);
        }
        return in;
    }

    inline QDataStream& InStreamSetManagerV3(QDataStream& in, mSetManager& mgr)
    {
        in >> mgr._maxID;
        int count;
        in >> count;
        for (int i = 0; i < count; ++i) {
            qint32 type;
            in >> type;
            mSetBase* obj = nullptr;
            switch ((MxFunctions::SetType)type)
            {
            case MxFunctions::Node:
                obj = new mNode;
                in >> *static_cast<mNode*>(obj);
                break;
            case MxFunctions::FrequencyPoint:
                obj = new mFrequencyPoint;
                in >> *static_cast<mFrequencyPoint*>(obj);
                break;
            case MxFunctions::DOFNode:
                obj = new mDOFNode;
                in >> *static_cast<mDOFNode*>(obj);
                break;
            case MxFunctions::Panel:
                obj = new mPanel;
                in >> *static_cast<mPanel*>(obj);
                break;
            case MxFunctions::Element:
                obj = new mElementSet;
                in >> *static_cast<mElementSet*>(obj);
                break;
            case MxFunctions::ElementSurface:
            case MxFunctions::ElementEdge:
                obj = new mElementSurface;
                in >> *static_cast<mElementSurface*>(obj);
                break;
            case MxFunctions::GeoEdge:
            case MxFunctions::GeoFace:
            case MxFunctions::GeoPoint:
            case MxFunctions::GeoSolid:
                obj = new mGeoSet;
                in >> *static_cast<mGeoSet*>(obj);
                break;
            case MxFunctions::Part:
                obj = new mPartSet;
                in >> *static_cast<mPartSet*>(obj);
                break;
            case MxFunctions::Mix:
                obj = new mMixSet;
                in >> *static_cast<mMixSet*>(obj);
                break;
            case MxFunctions::NodeSurface:
                obj = new mNodeFace;
                in >> *static_cast<mNodeFace*>(obj);
                break;
            case MxFunctions::SurfaceAnalysis:
                obj = new mSurfaceAnalysis;
                in >> *static_cast<mSurfaceAnalysis*>(obj);
                break;
            default:
                break;
            }

            if (obj)
                mgr.AddSetData(obj);
        }
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mSetBase& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamSetBaseV3(in, obj);
        }
        else
        {
            InStreamSetBaseV3(in, obj);
        }
        return in;
    }

    inline QDataStream& InStreamSetBaseV3(QDataStream& in, mSetBase& obj)
    {
        qint32 val;
        in >> val;
        obj._setType = static_cast<MxFunctions::SetType>(val);
        in >> obj._id >> obj._setName >> obj._internal >> obj._isUserRename;
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mNode& obj)
    {
        in >> static_cast<mSetBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamNodeV3(in, obj);
        }
        else
        {
            InStreamNodeV3(in, obj);
        }
        return in;
    }

    inline QDataStream& InStreamNodeV3(QDataStream& in, mNode& obj)
    {
        in >> obj._isOrder >> obj._ids;
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mPanel& obj)
    {
        in >> static_cast<mSetBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamPanelV3(in, obj);
        }
        else
        {
            InStreamPanelV3(in, obj);
        }
        return in;
    }

    inline QDataStream& InStreamPanelV3(QDataStream& in, mPanel& obj)
    {
        in >> obj._isOrder >> obj._ids >> obj._elenames >> obj._panelCollectionType >> obj._nodeNames;
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mFrequencyPoint& obj)
    {
        in >> static_cast<mSetBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamFrequencyPointV3(in, obj);
        }
        else
        {
            InStreamFrequencyPointV3(in, obj);
        }
        return in;
    }

    inline QDataStream& InStreamFrequencyPointV3(QDataStream& in, mFrequencyPoint& obj)
    {
        in >> obj._order;
        in >> obj._isOrder;
        in >> obj._ids;
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mDOFNode& obj)
    {
        in >> static_cast<mSetBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammDOFNodeV3(in, obj);
        }
        else
        {
            InStreammDOFNodeV3(in, obj);
        }
        return in;
    }

    inline QDataStream& InStreammDOFNodeV3(QDataStream& in, mDOFNode& obj)
    {
        in >> obj._idof >> obj._ids >> obj._isOrder;
        return in;
    }



    inline QDataStream& operator>>(QDataStream& in, mElementSet& obj)
    {
        in >> static_cast<mSetBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamElementV3(in, obj);
        }
        else
        {
            InStreamElementV3(in, obj);
        }
        return in;
    }

    inline QDataStream& InStreamElementV3(QDataStream& in, mElementSet& obj)
    {
        in >> obj._isOrder >> obj._ids;
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mElementSurface& obj)
    {
        in >> static_cast<mSetBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamElementSurfaceV3(in, obj);
        }
        else
        {
            InStreamElementSurfaceV3(in, obj);
        }
        return in;
    }

    inline QDataStream& InStreamElementSurfaceV3(QDataStream& in, mElementSurface& obj)
    {
        in  >> obj._ids;
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mGeoSet& obj)
    {
        in >> static_cast<mSetBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamGeoSetV3(in, obj);
        }
        else
        {
            InStreamGeoSetV3(in, obj);
        }
        return in;
    }

    inline QDataStream& InStreamGeoSetV3(QDataStream& in, mGeoSet& obj)
    {
        in >> obj._ids;
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mPartSet& obj)
    {
        in >> static_cast<mSetBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamPartSetV3(in, obj);
        }
        else
        {
            InStreamPartSetV3(in, obj);
        }
        return in;
    }

    inline QDataStream& InStreamPartSetV3(QDataStream& in, mPartSet& obj)
    {
        in >> obj._ids;
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMixSet& obj)
    {
        in >> static_cast<mSetBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamMixSetV3(in, obj);
        }
        else
        {
            InStreamMixSetV3(in, obj);
        }
        return in;
    }

    inline QDataStream& InStreamMixSetV3(QDataStream& in, mMixSet& obj)
    {
        in >> obj._ids;
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mNodeFace& obj)
    {
        in >> static_cast<mSetBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamNodeFaceV3(in, obj);
        }
        else
        {
            InStreamNodeFaceV3(in, obj);
        }
        return in;
    }

    inline QDataStream& InStreamNodeFaceV3(QDataStream& in, mNodeFace& obj)
    {
        in >> obj._ids;
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mSurfaceAnalysis& obj)
    {
        in >> static_cast<mSetBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamSurfaceAnalysisV3(in, obj);
        }
        else
        {
            InStreamSurfaceAnalysisV3(in, obj);
        }
        return in;
    }

    //原 InStreamNodeFaceV3(mSurfaceAnalysis&) 重载，改名以消除与 mNodeFace 版本函数的同名重载
    inline QDataStream& InStreamSurfaceAnalysisV3(QDataStream& in, mSurfaceAnalysis& obj)
    {
        // 先读子类型
        qint32 subType = 0;
        in >> subType;
        obj._subType = static_cast<SurfaceSubType>(subType);

        switch (obj._subType)
        {
        case RevolutionSurface:
        {
            in >> obj._xa >> obj._ya >> obj._za
                >> obj._xb >> obj._yb >> obj._zb
                >> obj._r1 >> obj._z1
                >> obj._r2 >> obj._z2;
            break;
        }
        case CylinderSurface:
        {
            in >> obj._ox >> obj._oy >> obj._oz
                >> obj._xx >> obj._xy >> obj._xz
                >> obj._zx >> obj._zy >> obj._zz
                >> obj._sx >> obj._sy
                >> obj._ex >> obj._ey >> obj._el;
            break;
        }
        case SegmentSurface:
        {
            in >> obj._ox >> obj._oy >> obj._oz
                >> obj._xx >> obj._xy >> obj._xz
                >> obj._zx >> obj._zy >> obj._zz
                >> obj._sx >> obj._sy;

            obj._contours.clear();
            int count = 0;
            in >> count;
            for (int i = 0; i < count; ++i)
            {
                SegmentContourRow row;
                in >> row.rowType >> row.x1 >> row.y1 >> row.x2 >> row.y2;
                obj._contours.append(row);
            }
            break;
        }
        default:
            break;
        }
        return in;
    }
}
