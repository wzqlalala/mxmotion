#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include <vector>

#include "mElementBase.h"
#include "mDataElement.h"
#include "mModelEnum.h"
#include "mModelStruct.h"

using namespace MxFunctions;
namespace MechElementData
{
    inline QDataStream& operator<<(QDataStream& out, mDataElement& mgr)
    {
        int count = mgr._elements.size();
        out << count;
        int elementID;
        qint32 type;
        for (auto it = mgr._elements.begin(); it != mgr._elements.end(); it++)
        {
            elementID = it.key();
            out << elementID;

            mElementBase* obj = it.value();
            MxFunctions::ElementType type = obj->getElementType();
            out << (qint32)type;
            switch (type)
            {
            case CPDSG:
            case T3:
            case TRIA3:
            case CTRIA3:
            case CQUAD4:
            case Q4:
            case MITC4:
            {
                out << *dynamic_cast<mShellElement*>(obj);
                break;
            }
            case Mass1:
            {
                out << *dynamic_cast<mMassElement*>(obj);
                break;
            }
            case CMASS2:
            {
                out << *dynamic_cast<mCmass2Element*>(obj);
                break;
            }
            case D3R2:
            case D2B2:
            case D2B3:
            case D3B2:
            case D3B3:
            {
                out << *dynamic_cast<mBeamElement*>(obj);
                break;
            }
            case ELAS1:
            {
                out << *dynamic_cast<mElas1Element*>(obj);
                break;
            }
            case CBUSH:
            {
                out << *dynamic_cast<mCbushElement*>(obj);
                break;
            }
            default:
                out << *obj;
                break;
            }
        }
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mElementBase& obj)
    {
        qint32 val = static_cast<MxFunctions::ElementType>(obj.elementType);
        out << val;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mMassElement& obj)
    {
        out << static_cast<mElementBase&>(obj);
        out << obj._mass << obj._x << obj._y << obj._z << obj._I11 << obj._I21 << obj._I22 << obj._I31
            << obj._I32 << obj._I33;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mCmass2Element& obj)
    {
        out << static_cast<mElementBase&>(obj);
        out << obj._mass << obj._dof;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mBeamElement& obj)
    {
        out << static_cast<mElementBase&>(obj);
        out << obj._beamValue << obj._node << obj._biasType << obj._X1 << obj._Y1 << obj._Z1 << obj._X2 << obj._Y2
            << obj._Z2 << obj._pinsa << obj._pinsb;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mShellElement& obj)
    {
        out << static_cast<mElementBase&>(obj);
        out << obj._paraType << obj._coordID << obj._angle << obj._bias<< obj._coorName;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mElas1Element& obj)
    {
        out << static_cast<mElementBase&>(obj);
        out << obj._dof;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mCbushElement& obj)
    {
        out << static_cast<mElementBase&>(obj);
        out << obj._cid << obj._s << obj._ocid << obj._x1 << obj._x2 << obj._x3 << obj._si << obj._cidName << obj._ocidName;
        return out;
    }

}
