#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

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
    inline QDataStream& operator>>(QDataStream& in, mSectionShapeManager& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammSectionShapeManagerV3(in, mgr);
        }
        else
        {
            InStreammSectionShapeManagerV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammSectionShapeManagerV3(QDataStream& in, mSectionShapeManager& mgr)
    {
        in >> mgr._maxID >> mgr._currentID;
        int count;
        in >> count;
        for (int i = 0; i < count; ++i) {
            qint32 type;
            in >> type;
            mSectionShapeBase* obj = nullptr;
            switch ((MxFunctions::SecType)type)
            {
            case MxFunctions::Sec_H:
                obj = new H_SectionVisual;
                in >> *dynamic_cast<H_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_I:
                obj = new I_SectionVisual;
                in >> *dynamic_cast<I_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_L:
                obj = new L_SectionVisual;
                in >> *dynamic_cast<L_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_T:
                obj = new T_SectionVisual;
                in >> *dynamic_cast<T_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Box:
                obj = new Box_SectionVisual;
                in >> *dynamic_cast<Box_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Rec:
                obj = new Rectangle_SectionVisual;
                in >> *dynamic_cast<Rectangle_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Cir:
                obj = new Circle_SectionVisual;
                in >> *dynamic_cast<Circle_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Ann:
                obj = new Annular_SectionVisual;
                in >> *dynamic_cast<Annular_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Channel:
                obj = new Channel_SectionVisual;
                in >> *dynamic_cast<Channel_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Channel1:
                obj = new Channel1_SectionVisual;
                in >> *dynamic_cast<Channel1_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Channel2:
                obj = new Channel2_SectionVisual;
                in >> *dynamic_cast<Channel2_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_I1:
                obj = new I1_SectionVisual;
                in >> *dynamic_cast<I1_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_T1:
                obj = new T1_SectionVisual;
                in >> *dynamic_cast<T1_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_T2:
                obj = new T2_SectionVisual;
                in >> *dynamic_cast<T2_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Box1:
                obj = new Box1_SectionVisual;
                in >> *dynamic_cast<Box1_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Cross:
                obj = new Cross_SectionVisual;
                in >> *dynamic_cast<Cross_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Hat:
                obj = new Hat_SectionVisual;
                in >> *dynamic_cast<Hat_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_Z:
                obj = new Z_SectionVisual;
                in >> *dynamic_cast<Z_SectionVisual*>(obj);
                break;
            case MxFunctions::Sec_None:
                obj = new mCustomSectionShape;
                in >> *dynamic_cast<mCustomSectionShape*>(obj);
                break;
            default:
                break;
            }

            if (obj)
                mgr.AddSectionShapeData(obj);
        }
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mSectionShapeBase& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammSectionShapeBaseV3(in, obj);
        }
        else
        {
            InStreammSectionShapeBaseV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammSectionShapeBaseV3(QDataStream& in, mSectionShapeBase& obj)
    {
        qint32 val;
        in >> val;
        obj._secType = static_cast<MxFunctions::SecType>(val);
        int _sectionShapeId;
        in >> _sectionShapeId;
        obj._sectionShapeId = _sectionShapeId;
        in >> obj._para >> obj._data >> obj.area >> obj.Iz >> obj.Iy >> obj.J >> obj.Iyz >> obj.K1 >> obj.K2 >> obj._id >> obj._sectionShapeName >> obj._nsm;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mCustomSectionShape& obj)
    {
        in >> static_cast<mSectionShapeBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammCustomSectionShapeV3(in, obj);
        }
        else
        {
            InStreammCustomSectionShapeV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammCustomSectionShapeV3(QDataStream& in, mCustomSectionShape& obj)
    {
        QList<QString> paras;
        in >> paras;
        obj.setCustomPara(paras);
        obj.CreateSec();
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, H_SectionVisual& obj)
    {
        in >> static_cast<mSectionShapeBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamH_SectionVisualV3(in, obj);
        }
        else
        {
            InStreamH_SectionVisualV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreamH_SectionVisualV3(QDataStream& in, H_SectionVisual& obj)
    {
        in >> obj.DIM1 >> obj.DIM2 >> obj.DIM3 >> obj.DIM4 >> obj.a >> obj.b >> obj.t2 >> obj.t1;
        in >> obj._X1 >> obj._Y1 >> obj._Z1 >> obj._X2 >> obj._Y2 >> obj._Z2 >> obj._X3 >> obj._Y3 >> obj._Z3;
        obj.CreateSec();
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, I_SectionVisual& obj)
    {
        in >> static_cast<mSectionShapeBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamI_SectionVisualV3(in, obj);
        }
        else
        {
            InStreamI_SectionVisualV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreamI_SectionVisualV3(QDataStream& in, I_SectionVisual& obj)
    {
        in >> obj.a >> obj.b >> obj.c>> obj.t1 >> obj.t2 >> obj.t3;
        in >> obj._Y1 >> obj._X1 >> obj._Z1 >> obj._Y2 >> obj._X2 >> obj._Z2 >> obj._Y3 >> obj._X3 >> obj._Z3;
        obj.CreateSec();
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, L_SectionVisual& obj)
    {
        in >> static_cast<mSectionShapeBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamL_SectionVisualV3(in, obj);
        }
        else
        {
            InStreamL_SectionVisualV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreamL_SectionVisualV3(QDataStream& in, L_SectionVisual& obj)
    {
        in >> obj.DIM1 >> obj.DIM2 >> obj.DIM3 >> obj.DIM4 >> obj.a >> obj.b >> obj.t1 >> obj.t2;
        in >> obj._Y >> obj._X >> obj._Z >> obj._name;
        obj.CreateSec();
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, T_SectionVisual& obj)
    {
        in >> static_cast<mSectionShapeBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamT_SectionVisualV3(in, obj);
        }
        else
        {
            InStreamT_SectionVisualV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreamT_SectionVisualV3(QDataStream& in, T_SectionVisual& obj)
    {
        in >> obj.a >> obj.b >> obj.c >> obj.t1 >> obj.t2;
        in >> obj._Y1 >> obj._X1 >> obj._Z1 >> obj._Y2 >> obj._X2 >> obj._Z2 >> obj._Y3 >> obj._X3 >> obj._Z3;
        obj.CreateSec();
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, Box_SectionVisual& obj)
    {
        in >> static_cast<mSectionShapeBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamBox_SectionVisualV3(in, obj);
        }
        else
        {
            InStreamBox_SectionVisualV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreamBox_SectionVisualV3(QDataStream& in, Box_SectionVisual& obj)
    {
        in >> obj.DIM1 >> obj.DIM2 >> obj.DIM3 >> obj.DIM4 >> obj.a >> obj.b >> obj.t1 >> obj.t2;
        in >> obj._Y >> obj._X >> obj._Z;
        obj.CreateSec();
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, Rectangle_SectionVisual& obj)
    {
        in >> static_cast<mSectionShapeBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamRectangle_SectionVisualV3(in, obj);
        }
        else
        {
            InStreamRectangle_SectionVisualV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreamRectangle_SectionVisualV3(QDataStream& in, Rectangle_SectionVisual& obj)
    {
        in >> obj.a >> obj.b;
        in >> obj._Y >> obj._X >> obj._Z;
        obj.CreateSec();
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, Circle_SectionVisual& obj)
    {
        in >> static_cast<mSectionShapeBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamCircle_SectionVisualV3(in, obj);
        }
        else
        {
            InStreamCircle_SectionVisualV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreamCircle_SectionVisualV3(QDataStream& in, Circle_SectionVisual& obj)
    {
        in >> obj._R;
        obj.CreateSec();
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, Annular_SectionVisual& obj)
    {
        in >> static_cast<mSectionShapeBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamAnnular_SectionVisualV3(in, obj);
        }
        else
        {
            InStreamAnnular_SectionVisualV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreamAnnular_SectionVisualV3(QDataStream& in, Annular_SectionVisual& obj)
    {
        in >> obj._R1 >> obj._R2;
        obj.CreateSec();
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, Channel_SectionVisual& obj)
    {
        in >> static_cast<mSectionShapeBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamChannel_SectionVisualV3(in, obj);
        }
        else
        {
            InStreamChannel_SectionVisualV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreamChannel_SectionVisualV3(QDataStream& in, Channel_SectionVisual& obj)
    {
        in >> obj.DIM3 >> obj.DIM4 >> obj.DIM1 >> obj.DIM2;
        in >> obj._X >> obj._Y >> obj._Z;
        obj.CreateSec();
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, Channel1_SectionVisual& obj)
    {
        in >> static_cast<mSectionShapeBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamChannel1_SectionVisualV3(in, obj);
        }
        else
        {
            InStreamChannel1_SectionVisualV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreamChannel1_SectionVisualV3(QDataStream& in, Channel1_SectionVisual& obj)
    {
        in >> obj.a >> obj.b >> obj.t1 >> obj.t2;
        in >> obj._X >> obj._Y >> obj._Z;
        obj.CreateSec();
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, Channel2_SectionVisual& obj)
    {
        in >> static_cast<mSectionShapeBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamChannel2_SectionVisualV3(in, obj);
        }
        else
        {
            InStreamChannel2_SectionVisualV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreamChannel2_SectionVisualV3(QDataStream& in, Channel2_SectionVisual& obj)
    {
        in >> obj.a >> obj.b >> obj.t1 >> obj.t2;
        in >> obj._X >> obj._Y >> obj._Z;
        obj.CreateSec();
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, I1_SectionVisual& obj)
    {
        in >> static_cast<mSectionShapeBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamI1_SectionVisualV3(in, obj);
        }
        else
        {
            InStreamI1_SectionVisualV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreamI1_SectionVisualV3(QDataStream& in, I1_SectionVisual& obj)
    {
        in >> obj.DIM1 >> obj.DIM2 >> obj.DIM3 >> obj.DIM4 >> obj.b >> obj.a >> obj.t1 >> obj.t2;
        in >> obj._Y1 >> obj._X1 >> obj._Z1 >> obj._Y2 >> obj._X2 >> obj._Z2 >> obj._Y3 >> obj._X3 >> obj._Z3;
        obj.CreateSec();
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, T1_SectionVisual& obj)
    {
        in >> static_cast<mSectionShapeBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamT1_SectionVisualV3(in, obj);
        }
        else
        {
            InStreamT1_SectionVisualV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreamT1_SectionVisualV3(QDataStream& in, T1_SectionVisual& obj)
    {
        in >> obj.DIM1 >> obj.DIM2 >> obj.DIM3 >> obj.DIM4;
        in >> obj._Y1 >> obj._X1 >> obj._Z1 >> obj._Y2 >> obj._X2 >> obj._Z2 >> obj._Y3 >> obj._X3 >> obj._Z3 >> obj._Y4 >> obj._X4 >> obj._Z4;
        obj.CreateSec();
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, T2_SectionVisual& obj)
    {
        in >> static_cast<mSectionShapeBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamT2_SectionVisualV3(in, obj);
        }
        else
        {
            InStreamT2_SectionVisualV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreamT2_SectionVisualV3(QDataStream& in, T2_SectionVisual& obj)
    {
        in >> obj.DIM1 >> obj.DIM2 >> obj.DIM3 >> obj.DIM4;
        in >> obj._Y1 >> obj._X1 >> obj._Z1 >> obj._Y2 >> obj._X2 >> obj._Z2 >> obj._Y3 >> obj._X3 >> obj._Z3 >> obj._Y4 >> obj._X4 >> obj._Z4;
        obj.CreateSec();
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, Box1_SectionVisual& obj)
    {
        in >> static_cast<mSectionShapeBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamBox1_SectionVisualV3(in, obj);
        }
        else
        {
            InStreamBox1_SectionVisualV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreamBox1_SectionVisualV3(QDataStream& in, Box1_SectionVisual& obj)
    {
        in >> obj.DIM1 >> obj.DIM2 >> obj.DIM3 >> obj.DIM4 >> obj.DIM5 >> obj.DIM6;
        obj.CreateSec();
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, Cross_SectionVisual& obj)
    {
        in >> static_cast<mSectionShapeBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamCross_SectionVisualV3(in, obj);
        }
        else
        {
            InStreamCross_SectionVisualV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreamCross_SectionVisualV3(QDataStream& in, Cross_SectionVisual& obj)
    {
        in >> obj.DIM1 >> obj.DIM2 >> obj.DIM3 >> obj.DIM4;
        obj.CreateSec();
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, Hat_SectionVisual& obj)
    {
        in >> static_cast<mSectionShapeBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamHat_SectionVisualV3(in, obj);
        }
        else
        {
            InStreamHat_SectionVisualV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreamHat_SectionVisualV3(QDataStream& in, Hat_SectionVisual& obj)
    {
        in >> obj.DIM1 >> obj.DIM2 >> obj.DIM3 >> obj.DIM4;
        obj.CreateSec();
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, Z_SectionVisual& obj)
    {
        in >> static_cast<mSectionShapeBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreamZ_SectionVisualV3(in, obj);
        }
        else
        {
            InStreamZ_SectionVisualV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreamZ_SectionVisualV3(QDataStream& in, Z_SectionVisual& obj)
    {
        in >> obj.DIM1 >> obj.DIM2 >> obj.DIM3 >> obj.DIM4;
        obj.CreateSec();
        return in;
    }


}
