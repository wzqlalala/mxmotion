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
    inline QDataStream& operator<<(QDataStream& out, mLoadBoundaryManager& mgr)
    {
        out << mgr._maxID;
        std::vector<int> loadIDs = mgr.getAllLoadBoundaryID();
        int count = loadIDs.size();
        out << count;
        for (int i = 0; i < count; ++i) {
            std::shared_ptr<mLoadBoundaryBase> obj = mgr.getLoadBoundaryData(loadIDs[i]);
            MxFunctions::LoadBoundaryType type = obj->getLoadBoundaryType();
            out << (qint32)type;
            switch (type)
            {
                //载荷
            case MxFunctions::LOAD_IM_FORCE:
            case MxFunctions::LOAD_IM_TORQUE:
                out << *std::dynamic_pointer_cast<mLoadForce>(obj);
                break;
            case MxFunctions::LOAD_IM_PRESSURE:
                out << *std::dynamic_pointer_cast<mLoadPressure>(obj);
                break;
            case MxFunctions::LOAD_IM_HEATFLUX:
                out << *std::dynamic_pointer_cast<mLoadHeatFlux>(obj);
                break;
            case MxFunctions::LOAD_IM_RADIATION:
                out << *std::dynamic_pointer_cast<mLoadRadiation>(obj);
                break;
            case MxFunctions::LOAD_IM_CONVECTION:
                out << *std::dynamic_pointer_cast<mLoadConvection>(obj);
                break;
            case MxFunctions::LOAD_IM_TRANSIENT:
                out << *std::dynamic_pointer_cast<mLoadTransient>(obj);
                break;
            case MxFunctions::LOAD_IM_FREQDOMAIN:
                out << *std::dynamic_pointer_cast<mLoadFreqdomain>(obj);
                break;
            case MxFunctions::LOAD_IM_COMBINED:
                out << *std::dynamic_pointer_cast<mLoadCombined>(obj);
                break;
            case MxFunctions::LOAD_IM_BOLT:
                out << *std::dynamic_pointer_cast<mLoadBolt>(obj);
                break;            
            case MxFunctions::LOAD_IM_ACOCONCFORCE:
                out << *std::dynamic_pointer_cast<mLoadAcousticConcForce>(obj);
                break;
            case MxFunctions::LOAD_IM_ACOFREQDOMAIN:
                out << *std::dynamic_pointer_cast<mLoadAcousticFrequencyDomain>(obj);
                break;
            case MxFunctions::LOAD_IM_SPECTRUM:
                out << *std::dynamic_pointer_cast<mLoadSpectrum>(obj);
                break;                        
            case MxFunctions::LOAD_ROTATE_PHYSICAL_POWER:
                out << *std::dynamic_pointer_cast<mRotatePhysicalPower>(obj);
                break;
            case MxFunctions::LOAD_IM_SURFACE:
                out << *std::dynamic_pointer_cast<mLoadSurface>(obj);
                break;
            case MxFunctions::LOAD_IM_EDGE:
                out << *std::dynamic_pointer_cast<mLoadEdge>(obj);
                break;

                //边界
            case MxFunctions::BOUNDARY_IM_FIXED:
                out << *std::dynamic_pointer_cast<mBoundaryFixed>(obj);
                break;
            case MxFunctions::BOUNDARY_IM_TEMP:
                out << *std::dynamic_pointer_cast<mBoundaryTemp>(obj);
                break;
            case MxFunctions::BOUNDARY_IM_DISPLACEMENT:
            case MxFunctions::BOUNDARY_IM_SPEED:
            case MxFunctions::BOUNDARY_IM_ACC:
                out << *std::dynamic_pointer_cast<mBoundaryDisplacement>(obj);
                break;
            case MxFunctions::BOUNDARY_MOVE:
                out << *std::dynamic_pointer_cast<mBoundaryMove>(obj);
                break;
            case MxFunctions::BOUNDARY_TEMP_NEW:
                out << *std::dynamic_pointer_cast<mBoundaryTempNew>(obj);
                break;
            case MxFunctions::BOUNDARY_IM_VIRTUAL:
                out << *std::dynamic_pointer_cast<mBoundaryVirtual>(obj);
                break;
            case MxFunctions::BOUNDARY_IM_SUPERELEMENT:
                out << *std::dynamic_pointer_cast<mBoundarySuperelement>(obj);
                break;
            case MxFunctions::LOAD_IM_F1:
                out << *std::dynamic_pointer_cast<mLoadF1>(obj);
                break;
            case MxFunctions::LOAD_IM_GRAVITY:
                out << *std::dynamic_pointer_cast<mLoadGravity>(obj);
                break;
            default:
                break;
            }
        }
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mLoadBoundaryBase& obj)
    {
        qint32 val = static_cast<MxFunctions::LoadBoundaryType>(obj._loadBoundaryType);
        out << val;
        out << obj._id << obj._loadBoundaryName << obj._isSuppressed << obj._pickState;
        return out;
    }

    //载荷
    inline QDataStream& operator<<(QDataStream& out, mLoadForce& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj.way_;
        out << obj.force_;
        out << obj.dir_;
        out << obj.force_x;
        out << obj.force_y;
        out << obj.force_z;
        out << obj.nodeForceMap_;
        out << obj.path_;
        out << obj.dirPickState_;
        out << obj.isAnalysisFieldMode_;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mLoadPressure& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj.pressureType_;
        out << obj.pa_;
        out << obj.way_;
        out << obj.cellFaceMap_;
        out << obj.path_;
        out << obj.depthDirection_;
        out << obj.depthDirPickState_;
        out << obj.zeroDepth_;
        out << obj.refDepth_;
        out << obj.refPressure_;
        out << obj.isAnalysisFieldMode_;
        return out;
    }
    
    inline QDataStream& operator<<(QDataStream& out, mLoadGravity& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj.way_;
        out << obj.magnitude_;
        out << obj.dir_;
        out << obj.V1;
        out << obj.V2;
        out << obj.V3;
        out << obj.isCornerAddChecked;
        out << obj.dirPickState_;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mLoadHeatFlux& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj.pa_;
        out << obj.way_;
        out << obj.defineType_;
        out << obj.cellFaceData_;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mLoadRadiation& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj.V1;
        out << obj.V2;
        out << obj.defineType_;
        out << obj.cellFaceData_;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mLoadConvection& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj.V1;
        out << obj.V2;
        out << obj.defineType_;
        out << obj.cellFaceData_;
        out << obj.path_;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mLoadSpectrum& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj.type_ << obj.dampValueList_ << obj.spectrumCurveList_;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mLoadTransient& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj.pa_ << obj.way_;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mLoadFreqdomain& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj.delay_;
        out << obj.phase_;
        out << obj.way2_;
        out << obj.way_;
        return out;
    }    
    
    inline QDataStream& operator<<(QDataStream& out, mLoadCombined& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj.ktotal_;
        out << obj.ratioList_;
        return out;
    }    
    
    inline QDataStream& operator<<(QDataStream& out, mLoadBolt& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj.way_;
        out << obj.value_;
        return out;
    }      
    
    inline QDataStream& operator<<(QDataStream& out, mLoadAcousticConcForce& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj.value_;
        return out;
    }    
    
    inline QDataStream& operator<<(QDataStream& out, mLoadAcousticFrequencyDomain& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj.delay_;
        out << obj.xita_;
        out << obj.rho_;
        out << obj.B_;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mRotatePhysicalPower& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj._startPoint1;
        out << obj._directPoint2;
        out << (qint32)obj._rotateType;
        out << (qint32)obj._defineType;
        out << obj._uniformValue;
        out << obj._elementCoorMap;
        out << obj._dirPickState;
        out << obj._picks;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mLoadEdge& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj.distributionType_;
        out << obj.defineWay_;
        out << obj.directionMode_;
        out << obj.size_;
        out << obj.dirPickState_;
        out << obj.dir_;
        return out;
    }

    //边界
    inline QDataStream& operator<<(QDataStream& out, mBoundaryFixed& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj._fixedType;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mBoundaryTemp& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj._temp;
        out << obj.isAnalysisFieldMode_;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mBoundaryDisplacement& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj._definitionMode;
        out << obj._U1 << obj._U2 << obj._U3 << obj._UR1 << obj._UR2 << obj._UR3;
        out << obj._independentTable;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mBoundaryMove& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj._mainModelFile << obj._timeStep << obj._mainSet << obj._useMainSet;
        out << obj._pickData;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mBoundaryTempNew& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj._mainModelFile << obj._timeStep << obj._mainSet << obj._useMainSet;
        out << obj._pickData;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mBoundaryVirtual& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj._U1 << obj._U2 << obj._U3 << obj._UR1 << obj._UR2 << obj._UR3;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mBoundarySuperelement& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj._ConstraintType << obj._DOF;
        return out;
    }    
    
    inline QDataStream& operator<<(QDataStream& out, mLoadSurface& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj.distributeType_ << obj.way_;
        out << obj.dirMode_;
        out << obj.loadValue_;
        out << obj.surfaceForceMap_;
        out << obj.dirPickState_;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mLoadF1& obj)
    {
        out << static_cast<mLoadBoundaryBase&>(obj);
        out << obj.type_ << obj.loadname_;
        out << obj.delay_ << obj.xita_;
        out << obj.curve1_ << obj.curve2_;
        return out;
    }
}




