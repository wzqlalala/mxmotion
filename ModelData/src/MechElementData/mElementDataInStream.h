#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mElementBase.h"
#include "mDataElement.h"
#include "mModelEnum.h"
#include "mModelStruct.h"

using namespace MxFunctions;
namespace MechElementData
{
    inline QDataStream& operator>>(QDataStream& in, mDataElement& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammDataElementV3(in, mgr);
        }
        else
        {
            InStreammDataElementV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammDataElementV3(QDataStream& in, mDataElement& mgr)
    {
        int count;
        in >> count;
        for (int i = 0; i < count; i++)
        {
            int elementID;
            in >> elementID;

            qint32 type;
            in >> type;
            mElementBase* obj = nullptr;
            switch ((MxFunctions::ElementType)type)
            {
            case CPDSG:
            case T3:
            case TRIA3:
            case CTRIA3:
            case CQUAD4:
            case Q4:
            case MITC4:
            {
                obj = new mShellElement;
                in >> *static_cast<mShellElement*>(obj);
                break;
            }
            case Mass1:
            {
                obj = new mMassElement;
                in >> *static_cast<mMassElement*>(obj);
                break;
            }
            case CMASS2:
            {
                obj = new mCmass2Element;
                in >> *static_cast<mCmass2Element*>(obj);
                break;
            }
            case D3R2:
            case D2B2:
            case D2B3:
            case D3B2:
            case D3B3:
            {
                obj = new mBeamElement;
                in >> *static_cast<mBeamElement*>(obj);
                break;
            }
            case ELAS1:
            {
                obj = new mElas1Element;
                in >> *static_cast<mElas1Element*>(obj);
                break;
            }
            case CBUSH:
            {
                obj = new mCbushElement;
                in >> *static_cast<mCbushElement*>(obj);
                break;
            }
            default:
                obj = new mElementBase;
                in >> *obj;
                break;
            }

            if(obj)
                mgr._elements[elementID] = obj;

        }
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mElementBase& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammElementBaseV3(in, obj);
        }
        else
        {
            InStreammElementBaseV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammElementBaseV3(QDataStream& in, mElementBase& obj)
    {
        qint32 val;
        in >> val;
        obj.elementType = static_cast<MxFunctions::ElementType>(val);
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mMassElement& obj)
    {
        in >> static_cast<mElementBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammMassElementV3(in, obj);
        }
        else
        {
            InStreammMassElementV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammMassElementV3(QDataStream& in, mMassElement& obj)
    {
        in >> obj._mass >> obj._x >> obj._y >> obj._z >> obj._I11 >> obj._I21 >> obj._I22 >> obj._I31
            >> obj._I32 >> obj._I33;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mCmass2Element& obj)
    {
        in >> static_cast<mElementBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammCmass2ElementV3(in, obj);
        }
        else
        {
            InStreammCmass2ElementV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammCmass2ElementV3(QDataStream& in, mCmass2Element& obj)
    {
        in >> obj._mass >> obj._dof;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mBeamElement& obj)
    {
        in >> static_cast<mElementBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammBeamElementV3(in, obj);
        }
        else
        {
            InStreammBeamElementV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammBeamElementV3(QDataStream& in, mBeamElement& obj)
    {
        in >> obj._beamValue >> obj._node >> obj._biasType >> obj._X1 >> obj._Y1 >> obj._Z1 >> obj._X2 >> obj._Y2
            >> obj._Z2 >> obj._pinsa >> obj._pinsb;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mShellElement& obj)
    {
        in >> static_cast<mElementBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammShellElementV3(in, obj);
        }
        else
        {
            InStreammShellElementV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammShellElementV3(QDataStream& in, mShellElement& obj)
    {
        in >> obj._paraType >> obj._coordID >> obj._angle >> obj._bias>>obj._coorName;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mElas1Element& obj)
    {
        in >> static_cast<mElementBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammElas1ElementV3(in, obj);
        }
        else
        {
            InStreammElas1ElementV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammElas1ElementV3(QDataStream& in, mElas1Element& obj)
    {
        in >> obj._dof;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mCbushElement& obj)
    {
        in >> static_cast<mElementBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammCbushElementV3(in, obj);
        }
        else
        {
            InStreammCbushElementV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammCbushElementV3(QDataStream& in, mCbushElement& obj)
    {
        in >> obj._cid >> obj._s >> obj._ocid >> obj._x1 >> obj._x2 >> obj._x3 >> obj._si>>obj._cidName>>obj._ocidName;
        return in;
    }

}
