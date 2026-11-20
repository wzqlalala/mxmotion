#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mPropertyManager.h"
#include "mPropertyBase.h"
#include "mConstantBeam.h"
#include "mCrossBeam.h"
#include "mShearPlate.h"
#include "mShellHomo.h"
#include "mSolidHomo.h"
#include "mTorsionBar.h"
#include "mTube.h"
#include "mSpring.h"
#include "mCbush.h"
#include "mViscousdampsection.h"
#include "mPlyLaminate.h"
#include "mMembrane.h"
#include "mGasket.h"
#include "mGapuni.h"
#include "mMacroEquivalent.h"

namespace MechPropertyData
{
    inline QDataStream& operator>>(QDataStream& in, mPropertyManager& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammPropertyManagerV3(in, mgr);
        }
        else
        {
            InStreammPropertyManagerV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammPropertyManagerV3(QDataStream& in, mPropertyManager& mgr)
    {
        in >> mgr._maxID;
        int count;
        in >> count;
        for (int i = 0; i < count; ++i) {
            qint32 type;
            in >> type;
            mPropertyBase* obj = nullptr;
            switch ((MxFunctions::PropertyType)type)
            {
            case MxFunctions::RodProperty:
                obj = new mTorsionBar;
                in >> *dynamic_cast<mTorsionBar*>(obj);
                break;
            case MxFunctions::ResultantProperty:
                obj = new mConstantBeam;
                in >> *dynamic_cast<mConstantBeam*>(obj);
                break;
            case MxFunctions::CrossProperty:
                obj = new mCrossBeam;
                in >> *dynamic_cast<mCrossBeam*>(obj);
                break;
            case MxFunctions::TubeProperty:
                obj = new mTube;
                in >> *dynamic_cast<mTube*>(obj);
                break;
            case MxFunctions::ShellProperty:
                obj = new mShellHomo;
                in >> *dynamic_cast<mShellHomo*>(obj);
                break;
            case MxFunctions::ShearPlateProperty:
                obj = new mShearPlate;
                in >> *dynamic_cast<mShearPlate*>(obj);
                break;
            case MxFunctions::SolidProperty:
                obj = new mSolidHomo;
                in >> *dynamic_cast<mSolidHomo*>(obj);
                break;
            case MxFunctions::Elas1Property:
            case MxFunctions::Spring1Property:
            case MxFunctions::Spring2Property:
            case MxFunctions::SpringAProperty:
                obj = new mSpring;
                in >> *dynamic_cast<mSpring*>(obj);
                break;
            case MxFunctions::ViscousDampProperty:
                obj = new mViscousdampsection;
                in >> *dynamic_cast<mViscousdampsection*>(obj);
                break;
            case MxFunctions::CbushProperty:
                obj = new mCbush;
                in >> *dynamic_cast<mCbush*>(obj);
                break;
            case MxFunctions::PlyLaminateProperty:
                obj = new mPlyLaminate;
                in >> *dynamic_cast<mPlyLaminate*>(obj);
                break;
            case MxFunctions::MembraneProperty:
                obj = new mMembrane;
                in >> *dynamic_cast<mMembrane*>(obj);
                break;
            case MxFunctions::GasketProperty:
                obj = new mGasket;
                in >> *dynamic_cast<mGasket*>(obj);
                break;
            case MxFunctions::GapuniProperty:
                obj = new mGapuni;
                in >> *dynamic_cast<mGapuni*>(obj);
                break;
            case MxFunctions::MacroEquivalentSection:
                obj = new mMacroEquivalent;
                in >> *dynamic_cast<mMacroEquivalent*>(obj);
                break;
            default:
                break;
            }

            if (obj)
                mgr.AddPropertyData(obj);
        }
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mPropertyBase& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammPropertyBaseV3(in, obj);
        }
        else
        {
            InStreammPropertyBaseV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammPropertyBaseV3(QDataStream& in, mPropertyBase& obj)
    {
        qint32 val;
        in >> val;
        obj._propertyType = static_cast<MxFunctions::PropertyType>(val);
        in >> obj._id >> obj._propertyName >> obj._propertyDescribe;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mTorsionBar& obj)
    {
        in >> static_cast<mPropertyBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammTorsionBarV3(in, obj);
        }
        else
        {
            InStreammTorsionBarV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammTorsionBarV3(QDataStream& in, mTorsionBar& obj)
    {
        in >> obj._J >> obj._A >> obj._nsm;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mConstantBeam& obj)
    {
        in >> static_cast<mPropertyBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammConstantBeamV3(in, obj);
        }
        else
        {
            InStreammConstantBeamV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammConstantBeamV3(QDataStream& in, mConstantBeam& obj)
    {
        in >> obj._angle;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mCrossBeam& obj)
    {
        in >> static_cast<mPropertyBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammCrossBeamV3(in, obj);
        }
        else
        {
            InStreammCrossBeamV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammCrossBeamV3(QDataStream& in, mCrossBeam& obj)
    {
        in >> obj._paraList;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mTube& obj)
    {
        in >> static_cast<mPropertyBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammTubeV3(in, obj);
        }
        else
        {
            InStreammTubeV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammTubeV3(QDataStream& in, mTube& obj)
    {
        in >> obj._od >> obj._t >> obj._nsm >> obj._od2;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mShellHomo& obj)
    {
        in >> static_cast<mPropertyBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammShellHomoV3(in, obj);
        }
        else
        {
            InStreammShellHomoV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammShellHomoV3(QDataStream& in, mShellHomo& obj)
    {
        in >> obj._thickness >> obj._nsm >> obj._referenceplane >> obj._z0;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mShearPlate& obj)
    {
        in >> static_cast<mPropertyBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammShearPlateV3(in, obj);
        }
        else
        {
            InStreammShearPlateV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammShearPlateV3(QDataStream& in, mShearPlate& obj)
    {
        in >> obj._thickness >> obj._valF1 >> obj._valF2;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mSolidHomo& obj)
    {
        in >> static_cast<mPropertyBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammSolidHomoV3(in, obj);
        }
        else
        {
            InStreammSolidHomoV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammSolidHomoV3(QDataStream& in, mSolidHomo& obj)
    {
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mSpring& obj)
    {
        in >> static_cast<mPropertyBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammSpringV3(in, obj);
        }
        else
        {
            InStreammSpringV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammSpringV3(QDataStream& in, mSpring& obj)
    {
        in >> obj._k >> obj._nonlinear >> obj._value >> obj._luCurve;
        in >> obj._K >> obj._GE >> obj._S;
        if (!in.atEnd())
            in >> obj._springCategory;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mViscousdampsection& obj)
    {
        in >> static_cast<mPropertyBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammViscousdampsectionV3(in, obj);
        }
        else
        {
            InStreammViscousdampsectionV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammViscousdampsectionV3(QDataStream& in, mViscousdampsection& obj)
    {
        in >> obj._GE;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mCbush& obj)
    {
        in >> static_cast<mPropertyBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammCbushV3(in, obj);
        }
        else
        {
            InStreammCbushV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammCbushV3(QDataStream& in, mCbush& obj)
    {
        in >> obj._isCheck >> obj._value;
        return in;
    }
    
    
    inline QDataStream& operator>>(QDataStream& in, mMembrane& obj)
    {
        in >> static_cast<mPropertyBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammMembraneV3(in, obj);
        }
        else
        {
            InStreammMembraneV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammMembraneV3(QDataStream& in, mMembrane& obj)
    {
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mGasket& obj)
    {
        in >> static_cast<mPropertyBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammGasketV3(in, obj);
        }
        else
        {
            InStreammGasketV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammGasketV3(QDataStream& in, mGasket& obj)
    {
        in >> obj._thicknessDef >> obj._initialThickness >> obj._initialGap;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mGapuni& obj)
    {
        in >> static_cast<mPropertyBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammGapuniV3(in, obj);
        }
        else
        {
            InStreammGapuniV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammGapuniV3(QDataStream& in, mGapuni& obj)
    {
        in >> obj._gapDistance >> obj._area >> obj._direction;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mPlyLaminate& obj)
    {
        in >> static_cast<mPropertyBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammPlyLaminateV3(in, obj);
        }
        else
        {
            InStreammPlyLaminateV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammPlyLaminateV3(QDataStream& in, mPlyLaminate& obj)
    {
        in >> obj._type >> obj._plyDefType;
        in >> obj._materialDir >> obj._normaldirection;
        in >> obj._rotAngle >> obj.thicknessList;
        in >> obj.angleList >> obj.intePointList;
        in >> obj._shellProperty >> obj._referenceSurfacePos;
        in >> obj._referenceSurfaceDis >> obj._failure;
        in >> obj._thickness >> obj._solidShellProperty;
        in >> obj._solidProperty >> obj.eleMentDir;
        in >> obj._rotAngleType;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mMacroEquivalent& obj)
    {
        in >> static_cast<mPropertyBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammMacroEquivalentV3(in, obj);
        }
        else
        {
            InStreammMacroEquivalentV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammMacroEquivalentV3(QDataStream& in, mMacroEquivalent& obj)
    {
        in >> obj._frbName;
        return in;
    }

}
