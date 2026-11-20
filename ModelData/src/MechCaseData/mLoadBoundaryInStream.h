#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mLoadBoundaryManager.h"
#include "mLoadBoundaryBase.h"

//载荷
#include "mLoadF1.h"
#include "mLoadForce.h"
#include "mLoadPressure.h"
#include "mLoadGravity.h"
#include "mLoadHeatFlux.h"
#include "mLoadRadiation.h"
#include "mLoadConvection.h"
#include "mLoadTransient.h"
#include "mLoadFreqdomain.h"
#include "mLoadCombined.h"
#include "mLoadSpectrum.h"
#include "mLoadBolt.h"
#include "mLoadAcousticConcForce.h"
#include "mLoadAcousticFrequencyDomain.h"
#include "mRotatePhysicalPower.h"
#include "mLoadSurface.h"
#include "mLoadEdge.h"

//边界
#include "mBoundaryDisplacement.h"
#include "mBoundaryFixed.h"
#include "mBoundaryVirtual.h"
#include "mBoundaryTemp.h"
#include "mBoundarySuperelement.h"
#include "mBoundaryMove.h"
#include "mBoundaryTempNew.h"


namespace MechCaseData
{
    inline QDataStream& operator>>(QDataStream& in, mLoadBoundaryManager& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLoadBoundaryManagerV3(in, mgr);
        }
        else
        {
            InStreammLoadBoundaryManagerV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammLoadBoundaryManagerV3(QDataStream& in, mLoadBoundaryManager& mgr)
    {
        in >> mgr._maxID;
        int count;
        in >> count;
        for (int i = 0; i < count; ++i) {
            qint32 type;
            in >> type;
            std::shared_ptr<mLoadBoundaryBase> obj = nullptr;
            switch ((MxFunctions::LoadBoundaryType)type)
            {
                //载荷
            case MxFunctions::LOAD_IM_FORCE:
            case MxFunctions::LOAD_IM_TORQUE:
                obj = std::make_shared<mLoadForce>();
                in >> *std::dynamic_pointer_cast<mLoadForce>(obj);
                break;
            case MxFunctions::LOAD_IM_PRESSURE:
                obj = std::make_shared<mLoadPressure>();
                in >> *std::dynamic_pointer_cast<mLoadPressure>(obj);
                break;            
            case MxFunctions::LOAD_IM_HEATFLUX:
                obj = std::make_shared<mLoadHeatFlux>();
                in >> *std::dynamic_pointer_cast<mLoadHeatFlux>(obj);
                break;            
            case MxFunctions::LOAD_IM_RADIATION:
                obj = std::make_shared<mLoadRadiation>();
                in >> *std::dynamic_pointer_cast<mLoadRadiation>(obj);
                break;            
            case MxFunctions::LOAD_IM_CONVECTION:
                obj = std::make_shared<mLoadConvection>();
                in >> *std::dynamic_pointer_cast<mLoadConvection>(obj);
                break;            
            case MxFunctions::LOAD_IM_TRANSIENT:
                obj = std::make_shared<mLoadTransient>();
                in >> *std::dynamic_pointer_cast<mLoadTransient>(obj);
                break;
            case MxFunctions::LOAD_IM_FREQDOMAIN:
                obj = std::make_shared<mLoadFreqdomain>();
                in >> *std::dynamic_pointer_cast<mLoadFreqdomain>(obj);
                break;            
            case MxFunctions::LOAD_IM_COMBINED:
                obj = std::make_shared<mLoadCombined>();
                in >> *std::dynamic_pointer_cast<mLoadCombined>(obj);
                break;
            case MxFunctions::LOAD_IM_BOLT:
                obj = std::make_shared<mLoadBolt>();
                in >> *std::dynamic_pointer_cast<mLoadBolt>(obj);
                break;            
            case MxFunctions::LOAD_IM_ACOCONCFORCE:
                obj = std::make_shared<mLoadAcousticConcForce>();
                in >> *std::dynamic_pointer_cast<mLoadAcousticConcForce>(obj);
                break;
            case MxFunctions::LOAD_IM_ACOFREQDOMAIN:
                obj = std::make_shared<mLoadAcousticFrequencyDomain>();
                in >> *std::dynamic_pointer_cast<mLoadAcousticFrequencyDomain>(obj);
                break;
            case MxFunctions::LOAD_IM_SPECTRUM:
                obj = std::make_shared<mLoadSpectrum>();
                in >> *std::dynamic_pointer_cast<mLoadSpectrum>(obj);
                break;            
            case MxFunctions::LOAD_ROTATE_PHYSICAL_POWER:
                obj = std::make_shared<mRotatePhysicalPower>();
                in >> *std::dynamic_pointer_cast<mRotatePhysicalPower>(obj);
                break;
            case MxFunctions::LOAD_IM_SURFACE:
                obj = std::make_shared<mLoadSurface>();
                in >> *std::dynamic_pointer_cast<mLoadSurface>(obj);
                break;
            case MxFunctions::LOAD_IM_EDGE:
                obj = std::make_shared<mLoadEdge>();
                in >> *std::dynamic_pointer_cast<mLoadEdge>(obj);
                break;
                //边界
            case MxFunctions::BOUNDARY_IM_FIXED:
                obj = std::make_shared<mBoundaryFixed>();
                in >> *std::dynamic_pointer_cast<mBoundaryFixed>(obj);
                break;
            case MxFunctions::BOUNDARY_IM_TEMP:
                obj = std::make_shared<mBoundaryTemp>();
                in >> *std::dynamic_pointer_cast<mBoundaryTemp>(obj);
                break;
            case MxFunctions::BOUNDARY_IM_DISPLACEMENT:
            case MxFunctions::BOUNDARY_IM_SPEED:
            case MxFunctions::BOUNDARY_IM_ACC:
                obj = std::make_shared<mBoundaryDisplacement>();
                in >> *std::dynamic_pointer_cast<mBoundaryDisplacement>(obj);
                break;
            case MxFunctions::BOUNDARY_MOVE:
                obj = std::make_shared<mBoundaryMove>();
                in >> *std::dynamic_pointer_cast<mBoundaryMove>(obj);
                break;
            case MxFunctions::BOUNDARY_TEMP_NEW:
                obj = std::make_shared<mBoundaryTempNew>();
                in >> *std::dynamic_pointer_cast<mBoundaryTempNew>(obj);
                break;
            case MxFunctions::BOUNDARY_IM_VIRTUAL:
                obj = std::make_shared<mBoundaryVirtual>();
                in >> *std::dynamic_pointer_cast<mBoundaryVirtual>(obj);
                break;
            case MxFunctions::BOUNDARY_IM_SUPERELEMENT:
                obj = std::make_shared<mBoundarySuperelement>();
                in >> *std::dynamic_pointer_cast<mBoundarySuperelement>(obj);
                break;
            case MxFunctions::LOAD_IM_F1:
                obj = std::make_shared<mLoadF1>();
                in >> *std::dynamic_pointer_cast<mLoadF1>(obj);
                break;
            case MxFunctions::LOAD_IM_GRAVITY:
                obj = std::make_shared<mLoadGravity>();
                in >> *std::dynamic_pointer_cast<mLoadGravity>(obj);
                break;
            default:
                break;
            }

            if (obj)
                mgr.AddLoadBoundaryData(obj);
        }
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mLoadBoundaryBase& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLoadBoundaryBaseV3(in, obj);
        }
        else
        {
            InStreammLoadBoundaryBaseV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammLoadBoundaryBaseV3(QDataStream& in, mLoadBoundaryBase& obj)
    {
        qint32 val;
        in >> val;
        obj._loadBoundaryType = static_cast<MxFunctions::LoadBoundaryType>(val);
        in >> obj._id >> obj._loadBoundaryName >> obj._isSuppressed >> obj._pickState;
        return in;
    }


    //载荷
    inline QDataStream& operator>>(QDataStream& in, mLoadForce& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLoadForceV3(in, obj);
        }
        else
        {
            InStreammLoadForceV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammLoadForceV3(QDataStream& in, mLoadForce& obj)
    {
        in >> obj.way_;
        in >> obj.force_;
        in >> obj.dir_;
        in >> obj.force_x;
        in >> obj.force_y;
        in >> obj.force_z;
        in >> obj.nodeForceMap_;
        in >> obj.path_;
        in >> obj.dirPickState_;
        in >> obj.isAnalysisFieldMode_;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mLoadPressure& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLoadPressureV3(in, obj);
        }
        else
        {
            InStreammLoadPressureV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammLoadPressureV3(QDataStream& in, mLoadPressure& obj)
    {
        in >> obj.pressureType_;
        in >> obj.pa_;
        in >> obj.way_;
        in >> obj.cellFaceMap_;
        in >> obj.path_;
        in >> obj.depthDirection_;
        in >> obj.depthDirPickState_;
        in >> obj.zeroDepth_;
        in >> obj.refDepth_;
        in >> obj.refPressure_;
        in >> obj.isAnalysisFieldMode_;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mLoadGravity& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLoadGravityV3(in, obj);
        }
        else
        {
            InStreammLoadGravityV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammLoadGravityV3(QDataStream& in, mLoadGravity& obj)
    {
        in >> obj.way_;
        in >> obj.magnitude_;
        in >> obj.dir_;
        in >> obj.V1;
        in >> obj.V2;
        in >> obj.V3;
        in >> obj.isCornerAddChecked;
        in >> obj.dirPickState_;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mLoadHeatFlux& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLoadHeatFluxV3(in, obj);
        }
        else
        {
            InStreammLoadHeatFluxV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammLoadHeatFluxV3(QDataStream& in, mLoadHeatFlux& obj)
    {
        in >> obj.pa_;
        in >> obj.way_;
        in >> obj.defineType_;
        in >> obj.cellFaceData_;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mLoadRadiation& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLoadRadiationV3(in, obj);
        }
        else
        {
            InStreammLoadRadiationV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammLoadRadiationV3(QDataStream& in, mLoadRadiation& obj)
    {
        in >> obj.V1;
        in >> obj.V2;
        in >> obj.defineType_;
        in >> obj.cellFaceData_;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mLoadConvection& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLoadConvectionV3(in, obj);
        }
        else
        {
            InStreammLoadConvectionV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammLoadConvectionV3(QDataStream& in, mLoadConvection& obj)
    {
        in >> obj.V1;
        in >> obj.V2;
        in >> obj.defineType_;
        in >> obj.cellFaceData_;
        in >> obj.path_;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mLoadTransient& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLoadTransientV3(in, obj);
        }
        else
        {
            InStreammLoadTransientV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammLoadTransientV3(QDataStream& in, mLoadTransient& obj)
    {
        in >> obj.pa_ >> obj.way_;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mLoadSpectrum& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLoadSpectrumV3(in, obj);
        }
        else
        {
            InStreammLoadSpectrumV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammLoadSpectrumV3(QDataStream& in, mLoadSpectrum& obj)
    {
        in >> obj.type_ >> obj.dampValueList_ >> obj.spectrumCurveList_;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mLoadFreqdomain& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLoadFreqdomainV3(in, obj);
        }
        else
        {
            InStreammLoadFreqdomainV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammLoadFreqdomainV3(QDataStream& in, mLoadFreqdomain& obj)
    {
        in >> obj.delay_;
        in >> obj.phase_;
        in >> obj.way2_;
        in >> obj.way_;
        return in;
    }

    
    inline QDataStream& operator>>(QDataStream& in, mLoadCombined& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLoadCombinedV3(in, obj);
        }
        else
        {
            InStreammLoadCombinedV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammLoadCombinedV3(QDataStream& in, mLoadCombined& obj)
    {
        in >> obj.ktotal_;
        in >> obj.ratioList_;
        return in;
    }
    
    
    inline QDataStream& operator>>(QDataStream& in, mLoadBolt& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLoadBoltV3(in, obj);
        }
        else
        {
            InStreammLoadBoltV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammLoadBoltV3(QDataStream& in, mLoadBolt& obj)
    {
        in >> obj.way_;
        in >> obj.value_;
        return in;
    }
    
    
    inline QDataStream& operator>>(QDataStream& in, mLoadAcousticConcForce& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLoadAcousticConcForceV3(in, obj);
        }
        else
        {
            InStreammLoadAcousticConcForceV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammLoadAcousticConcForceV3(QDataStream& in, mLoadAcousticConcForce& obj)
    {
        in >> obj.value_;
        return in;
    }
    
    
    inline QDataStream& operator>>(QDataStream& in, mLoadAcousticFrequencyDomain& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLoadAcousticFrequencyDomainV3(in, obj);
        }
        else
        {
            InStreammLoadAcousticFrequencyDomainV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammLoadAcousticFrequencyDomainV3(QDataStream& in, mLoadAcousticFrequencyDomain& obj)
    {
        in >> obj.delay_;
        in >> obj.xita_;
        in >> obj.rho_;
        in >> obj.B_;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mRotatePhysicalPower& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammRotatePhysicalPowerV3(in, obj);
        }
        else
        {
            InStreammRotatePhysicalPowerV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammRotatePhysicalPowerV3(QDataStream& in, mRotatePhysicalPower& obj)
    {
        in >> obj._startPoint1;
        in >> obj._directPoint2;
        qint32 rotateType = 0;
        in >> rotateType;
        obj._rotateType = rotateType;
        qint32 defineType = 0;
        in >> defineType;
        obj._defineType = defineType;
        in >> obj._uniformValue;
        in >> obj._elementCoorMap;
        in >> obj._dirPickState;
        in >> obj._picks;
        return in;
    }
    
    
    inline QDataStream& operator>>(QDataStream& in, mLoadEdge& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLoadEdgeV3(in, obj);
        }
        else
        {
            InStreammLoadEdgeV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammLoadEdgeV3(QDataStream& in, mLoadEdge& obj)
    {
        in >> obj.distributionType_;
        in >> obj.defineWay_;
        in >> obj.directionMode_;
        in >> obj.size_;
        in >> obj.dirPickState_;
        in >> obj.dir_;
        return in;
    }


    //边界
    inline QDataStream& operator>>(QDataStream& in, mBoundaryFixed& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammBoundaryFixedV3(in, obj);
        }
        else
        {
            InStreammBoundaryFixedV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammBoundaryFixedV3(QDataStream& in, mBoundaryFixed& obj)
    {
        in >> obj._fixedType;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mBoundaryTemp& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammBoundaryTempV3(in, obj);
        }
        else
        {
            InStreammBoundaryTempV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammBoundaryTempV3(QDataStream& in, mBoundaryTemp& obj)
    {
        in >> obj._temp;
        in >> obj.isAnalysisFieldMode_;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mBoundaryDisplacement& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammBoundaryDisplacementV3(in, obj);
        }
        else
        {
            InStreammBoundaryDisplacementV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammBoundaryDisplacementV3(QDataStream& in, mBoundaryDisplacement& obj)
    {
        in >> obj._definitionMode;
        in >> obj._U1 >> obj._U2 >> obj._U3 >> obj._UR1 >> obj._UR2 >> obj._UR3;
        in >> obj._independentTable;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mBoundaryMove& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammBoundaryMoveV3(in, obj);
        }
        else
        {
            InStreammBoundaryMoveV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammBoundaryMoveV3(QDataStream& in, mBoundaryMove& obj)
    {
        in >> obj._mainModelFile >> obj._timeStep >> obj._mainSet >> obj._useMainSet;
        in >> obj._pickData;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mBoundaryTempNew& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammBoundaryTempNewV3(in, obj);
        }
        else
        {
            InStreammBoundaryTempNewV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammBoundaryTempNewV3(QDataStream& in, mBoundaryTempNew& obj)
    {
        in >> obj._mainModelFile >> obj._timeStep >> obj._mainSet >> obj._useMainSet;
        in >> obj._pickData;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mBoundaryVirtual& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammBoundaryVirtualV3(in, obj);
        }
        else
        {
            InStreammBoundaryVirtualV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammBoundaryVirtualV3(QDataStream& in, mBoundaryVirtual& obj)
    {
        in >> obj._U1 >> obj._U2 >> obj._U3 >> obj._UR1 >> obj._UR2 >> obj._UR3;
        return in;
    }
    
    
    inline QDataStream& operator>>(QDataStream& in, mBoundarySuperelement& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammBoundarySuperelementV3(in, obj);
        }
        else
        {
            InStreammBoundarySuperelementV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammBoundarySuperelementV3(QDataStream& in, mBoundarySuperelement& obj)
    {
        in >> obj._ConstraintType >> obj._DOF;
        return in;
    }
    
    
    inline QDataStream& operator>>(QDataStream& in, mLoadSurface& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLoadSurfaceV3(in, obj);
        }
        else
        {
            InStreammLoadSurfaceV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammLoadSurfaceV3(QDataStream& in, mLoadSurface& obj)
    {
        in >> obj.distributeType_ >> obj.way_;
        in >> obj.dirMode_;
        in >> obj.loadValue_;
        in >> obj.surfaceForceMap_;
        in >> obj.dirPickState_;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mLoadF1& obj)
    {
        in >> static_cast<mLoadBoundaryBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammLoadF1V3(in, obj);
        }
        else
        {
            InStreammLoadF1V3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammLoadF1V3(QDataStream& in, mLoadF1& obj)
    {
        in >> obj.type_ >> obj.loadname_;
        in >> obj.delay_ >> obj.xita_;
        in >> obj.curve1_ >> obj.curve2_;
        return in;
    }


}
