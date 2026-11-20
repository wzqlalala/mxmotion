#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include <vector>

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
    inline QDataStream& operator<<(QDataStream& out, mPropertyManager& mgr)
    {
        out << mgr._maxID;
        std::vector<int> propertyIDs = mgr.getAllProperyID();
        int count = propertyIDs.size();
        out << count;
        for (int i = 0; i < count; ++i) {
            mPropertyBase* obj = mgr.getPropertyData(propertyIDs[i]);
            MxFunctions::PropertyType type = obj->getPropertyType();
            out << (qint32)type;
            switch (type)
            {
            case MxFunctions::RodProperty:
                out << *dynamic_cast<mTorsionBar*>(obj);
                break;
            case MxFunctions::ResultantProperty:
                out << *dynamic_cast<mConstantBeam*>(obj);
                break;
            case MxFunctions::CrossProperty:
                out << *dynamic_cast<mCrossBeam*>(obj);
                break;
            case MxFunctions::TubeProperty:
                out << *dynamic_cast<mTube*>(obj);
                break;
            case MxFunctions::ShellProperty:
                out << *dynamic_cast<mShellHomo*>(obj);
                break;
            case MxFunctions::ShearPlateProperty:
                out << *dynamic_cast<mShearPlate*>(obj);
                break;
            case MxFunctions::SolidProperty:
                out << *dynamic_cast<mSolidHomo*>(obj);
                break;
            case MxFunctions::Elas1Property:
            case MxFunctions::Spring1Property:
            case MxFunctions::Spring2Property:
            case MxFunctions::SpringAProperty:
                out << *dynamic_cast<mSpring*>(obj);
                break;
            case MxFunctions::ViscousDampProperty:
                out << *dynamic_cast<mViscousdampsection*>(obj);
                break;
            case MxFunctions::CbushProperty:
                out << *dynamic_cast<mCbush*>(obj);
                break;
            case MxFunctions::PlyLaminateProperty:
                out << *dynamic_cast<mPlyLaminate*>(obj);
                break;
            case MxFunctions::MembraneProperty:
                out << *dynamic_cast<mMembrane*>(obj);
                break;
            case MxFunctions::GasketProperty:
                out << *dynamic_cast<mGasket*>(obj);
                break;
            case MxFunctions::GapuniProperty:
                out << *dynamic_cast<mGapuni*>(obj);
                break;
            case MxFunctions::MacroEquivalentSection:
                out << *dynamic_cast<mMacroEquivalent*>(obj);
                break;
            default:
                break;
            }
        }
        return out;
    }


    inline QDataStream& operator<<(QDataStream& out, mPropertyBase& obj)
    {
        qint32 val = static_cast<MxFunctions::CaseType>(obj._propertyType);
        out << val;
        out << obj._id << obj._propertyName << obj._propertyDescribe;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mTorsionBar& obj)
    {
        out << static_cast<mPropertyBase&>(obj);
        out << obj._J << obj._A << obj._nsm;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mConstantBeam& obj)
    {
        out << static_cast<mPropertyBase&>(obj);
        out << obj._angle;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mCrossBeam& obj)
    {
        out << static_cast<mPropertyBase&>(obj);
        out << obj._paraList;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mTube& obj)
    {
        out << static_cast<mPropertyBase&>(obj);
        out << obj._od << obj._t << obj._nsm << obj._od2;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mShellHomo& obj)
    {
        out << static_cast<mPropertyBase&>(obj);
        out << obj._thickness << obj._nsm << obj._referenceplane << obj._z0;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mShearPlate& obj)
    {
        out << static_cast<mPropertyBase&>(obj);
        out << obj._thickness << obj._valF1 << obj._valF2;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mSolidHomo& obj)
    {
        out << static_cast<mPropertyBase&>(obj);
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mSpring& obj)
    {
        out << static_cast<mPropertyBase&>(obj);
        out << obj._k << obj._nonlinear << obj._value << obj._luCurve;
        out << obj._K << obj._GE << obj._S;
        out << obj._springCategory;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mViscousdampsection& obj)
    {
        out << static_cast<mPropertyBase&>(obj);
        out << obj._GE;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mCbush& obj)
    {
        out << static_cast<mPropertyBase&>(obj);
        out << obj._isCheck << obj._value;
        return out;
    }    
    
    inline QDataStream& operator<<(QDataStream& out, mMembrane& obj)
    {
        out << static_cast<mPropertyBase&>(obj);
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mGasket& obj)
    {
        out << static_cast<mPropertyBase&>(obj);
        out << obj._thicknessDef << obj._initialThickness << obj._initialGap;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mGapuni& obj)
    {
        out << static_cast<mPropertyBase&>(obj);
        out << obj._gapDistance << obj._area << obj._direction;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mPlyLaminate& obj)
    {
        out << static_cast<mPropertyBase&>(obj);
        out << obj._type << obj._plyDefType;
        out << obj._materialDir << obj._normaldirection;
        out << obj._rotAngle << obj.thicknessList;
        out << obj.angleList << obj.intePointList;
        out << obj._shellProperty << obj._referenceSurfacePos;
        out << obj._referenceSurfaceDis << obj._failure;
        out << obj._thickness << obj._solidShellProperty;
        out << obj._solidProperty << obj.eleMentDir;
        out << obj._rotAngleType;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mMacroEquivalent& obj)
    {
        out << static_cast<mPropertyBase&>(obj);
        out << obj._frbName;
        return out;
    }
}
