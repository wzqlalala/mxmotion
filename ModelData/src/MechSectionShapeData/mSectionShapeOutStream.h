#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include <vector>

#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mSectionShapeManager.h"
#include "mSectionShapeBase.h"
#include "Annular_SectionVisual.h"
#include "Box_SectionVisual.h"
#include "Box1_SectionVisual.h"
#include "Channel_SectionVisual.h"
#include "Channel1_SectionVisual.h"
#include "Channel2_SectionVisual.h"
#include "Circle_SectionVisual.h"
#include "Cross_SectionVisual.h"
#include "Hat_SectionVisual.h"
#include "H_SectionVisual.h"
#include "I_SectionVisual.h"
#include "I1_SectionVisual.h"
#include "L_SectionVisual.h"
#include "Rectangle_SectionVisual.h"
#include "T_SectionVisual.h"
#include "T1_SectionVisual.h"
#include "T2_SectionVisual.h"
#include "Z_SectionVisual.h"
#include "mCustomSectionShape.h"

namespace MechSectionShapeData
{
    inline QDataStream& operator<<(QDataStream& out, mSectionShapeManager& mgr)
    {
        out << mgr._maxID << mgr._currentID;
        std::vector<int> shapeIDs = mgr.getAllSectionShapeID();
        int count = shapeIDs.size();
        out << count;
        for (int i = 0; i < count; ++i) {
            mSectionShapeBase* obj = mgr.getSectionShapeData(shapeIDs[i]);
            MxFunctions::SecType type = obj->getSectionShapeType();
            out << (qint32)type;
            switch ((MxFunctions::SecType)type)
            {
            case MxFunctions::Sec_H:
                out << *dynamic_cast<H_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_I:
                out << *dynamic_cast<I_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_L:
                out << *dynamic_cast<L_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_T:
                out << *dynamic_cast<T_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Box:
                out << *dynamic_cast<Box_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Rec:
                out << *dynamic_cast<Rectangle_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Cir:
                out << *dynamic_cast<Circle_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Ann:
                out << *dynamic_cast<Annular_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Channel:
                out << *dynamic_cast<Channel_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Channel1:
                out << *dynamic_cast<Channel1_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Channel2:
                out << *dynamic_cast<Channel2_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_I1:
                out << *dynamic_cast<I1_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_T1:
                out << *dynamic_cast<T1_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_T2:
                out << *dynamic_cast<T2_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Box1:
                out << *dynamic_cast<Box1_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Cross:
                out << *dynamic_cast<Cross_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Hat:
                out << *dynamic_cast<Hat_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Z:
                out << *dynamic_cast<Z_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_None:
                out << *dynamic_cast<mCustomSectionShape*>(obj);
                break;
            default:
                break;
            }
        }
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mSectionShapeBase& obj)
    {
        qint32 val = static_cast<MxFunctions::SecType>(obj._secType);
        out << val << obj._sectionShapeId;
        out << obj._para << obj._data << obj.area << obj.Iz << obj.Iy << obj.J << obj.Iyz << obj.K1 << obj.K2 << obj._id << obj._sectionShapeName << obj._nsm;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mCustomSectionShape& obj)
    {
        out << static_cast<mSectionShapeBase&>(obj);
        out << obj.getCustomPara();
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, H_SectionVisual& obj)
    {
        out << static_cast<mSectionShapeBase&>(obj);
        out << obj.DIM1 << obj.DIM2 << obj.DIM3 << obj.DIM4 << obj.a << obj.b << obj.t2 << obj.t1;
        out << obj._X1 << obj._Y1 << obj._Z1 << obj._X2 << obj._Y2 << obj._Z2 << obj._X3 << obj._Y3 << obj._Z3;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, I_SectionVisual& obj)
    {
        out << static_cast<mSectionShapeBase&>(obj);
        out << obj.a << obj.b << obj.c<< obj.t1 << obj.t2 << obj.t3;
        out << obj._Y1 << obj._X1 << obj._Z1 << obj._Y2 << obj._X2 << obj._Z2 << obj._Y3 << obj._X3 << obj._Z3;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, L_SectionVisual& obj)
    {
        out << static_cast<mSectionShapeBase&>(obj);
        out << obj.DIM1 << obj.DIM2 << obj.DIM3 << obj.DIM4 << obj.a << obj.b << obj.t1 << obj.t2;
        out << obj._Y << obj._X << obj._Z << obj._name;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, T_SectionVisual& obj)
    {
        out << static_cast<mSectionShapeBase&>(obj);
        out << obj.a << obj.b << obj.c << obj.t1 << obj.t2;
        out << obj._Y1 << obj._X1 << obj._Z1 << obj._Y2 << obj._X2 << obj._Z2 << obj._Y3 << obj._X3 << obj._Z3;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, Box_SectionVisual& obj)
    {
        out << static_cast<mSectionShapeBase&>(obj);
        out << obj.DIM1 << obj.DIM2 << obj.DIM3 << obj.DIM4 << obj.a << obj.b << obj.t1 << obj.t2;
        out << obj._Y << obj._X << obj._Z;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, Rectangle_SectionVisual& obj)
    {
        out << static_cast<mSectionShapeBase&>(obj);
        out << obj.a << obj.b;
        out << obj._Y << obj._X << obj._Z;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, Circle_SectionVisual& obj)
    {
        out << static_cast<mSectionShapeBase&>(obj);
        out << obj._R;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, Annular_SectionVisual& obj)
    {
        out << static_cast<mSectionShapeBase&>(obj);
        out << obj._R1 << obj._R2;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, Channel_SectionVisual& obj)
    {
        out << static_cast<mSectionShapeBase&>(obj);
        out << obj.DIM3 << obj.DIM4 << obj.DIM1 << obj.DIM2;
        out << obj._X << obj._Y << obj._Z;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, Channel1_SectionVisual& obj)
    {
        out << static_cast<mSectionShapeBase&>(obj);
        out << obj.a << obj.b << obj.t1 << obj.t2;
        out << obj._X << obj._Y << obj._Z;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, Channel2_SectionVisual& obj)
    {
        out << static_cast<mSectionShapeBase&>(obj);
        out << obj.a << obj.b << obj.t1 << obj.t2;
        out << obj._X << obj._Y << obj._Z;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, I1_SectionVisual& obj)
    {
        out << static_cast<mSectionShapeBase&>(obj);
        out << obj.DIM1 << obj.DIM2 << obj.DIM3 << obj.DIM4 << obj.b << obj.a << obj.t1 << obj.t2;
        out << obj._Y1 << obj._X1 << obj._Z1 << obj._Y2 << obj._X2 << obj._Z2 << obj._Y3 << obj._X3 << obj._Z3;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, T1_SectionVisual& obj)
    {
        out << static_cast<mSectionShapeBase&>(obj);
        out << obj.DIM1 << obj.DIM2 << obj.DIM3 << obj.DIM4;
        out << obj._Y1 << obj._X1 << obj._Z1 << obj._Y2 << obj._X2 << obj._Z2 << obj._Y3 << obj._X3 << obj._Z3 << obj._Y4 << obj._X4 << obj._Z4;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, T2_SectionVisual& obj)
    {
        out << static_cast<mSectionShapeBase&>(obj);
        out << obj.DIM1 << obj.DIM2 << obj.DIM3 << obj.DIM4;
        out << obj._Y1 << obj._X1 << obj._Z1 << obj._Y2 << obj._X2 << obj._Z2 << obj._Y3 << obj._X3 << obj._Z3 << obj._Y4 << obj._X4 << obj._Z4;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, Box1_SectionVisual& obj)
    {
        out << static_cast<mSectionShapeBase&>(obj);
        out << obj.DIM1 << obj.DIM2 << obj.DIM3 << obj.DIM4 << obj.DIM5 << obj.DIM6;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, Cross_SectionVisual& obj)
    {
        out << static_cast<mSectionShapeBase&>(obj);
        out << obj.DIM1 << obj.DIM2 << obj.DIM3 << obj.DIM4;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, Hat_SectionVisual& obj)
    {
        out << static_cast<mSectionShapeBase&>(obj);
        out << obj.DIM1 << obj.DIM2 << obj.DIM3 << obj.DIM4;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, Z_SectionVisual& obj)
    {
        out << static_cast<mSectionShapeBase&>(obj);
        out << obj.DIM1 << obj.DIM2 << obj.DIM3 << obj.DIM4;
        return out;
    }

}