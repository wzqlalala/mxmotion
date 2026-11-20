#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include <QList>

#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mCaseManager.h"
#include "mCaseBase.h"
#include "mCaseImplicitResponseSpectrum.h"
#include "mCaseImplicitRandomVibration.h"
#include "mCaseImplicitFrequency.h"
#include "mCaseImplASCoupling.h"
#include "mCaseImplicitLinearBuckling.h"
#include "mCaseImplicitLinearStatic.h"
#include "mCaseImplicitModal.h"
#include "mCaseImplicitNonLinearArc.h"
#include "mCaseImplicitNonLinearDynamic.h"
#include "mCaseImplicitNonLinearStatic.h"
#include "mCaseImplicitSteadyHeat.h"
#include "mCaseImplicitTransient.h"
#include "mCaseImplicitTransientHeat.h"
#include "mCaseImplicitStructuralCouplingHeat.h"
#include "mCaseImplicitGeneric.h"

namespace MechCaseData
{
	inline QDataStream& operator>>(QDataStream& in, mCaseManager& mgr)
	{
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammCaseManagerV3(in, mgr);
	    }
	    else
	    {
	        InStreammCaseManagerV3(in, mgr);
	    }
	    return in;
}
	inline QDataStream& InStreammCaseManagerV3(QDataStream& in, mCaseManager& mgr)
	{
		in >> mgr._maxID;
		int count;
		in >> count;
		for (int i = 0; i < count; ++i) {
			qint32 type;
			in >> type;
			mCaseBase* obj = nullptr;
			switch ((MxFunctions::CaseType)type)
			{
			case MxFunctions::ImplicitLinearStatic:
				obj = new mCaseImplicitLinearStatic;
				in >> *dynamic_cast<mCaseImplicitLinearStatic*>(obj);
				break;
			case MxFunctions::ImplicitNonLinearStatic:
				obj = new mCaseImplicitNonLinearStatic;
				in >> *dynamic_cast<mCaseImplicitNonLinearStatic*>(obj);
				break;
			case MxFunctions::ImplicitModal:
				obj = new mCaseImplicitModal;
				in >> *dynamic_cast<mCaseImplicitModal*>(obj);
				break;
			case MxFunctions::ImplicitResponseSpectrum:
				obj = new mCaseImplicitResponseSpectrum;
				in >> *dynamic_cast<mCaseImplicitResponseSpectrum*>(obj);
				break;
			case MxFunctions::ImplicitRandomVibration:
				obj = new mCaseImplicitRandomVibration;
				in >> *dynamic_cast<mCaseImplicitRandomVibration*>(obj);
				break;
			case MxFunctions::ImplicitFrequency:
				obj = new mCaseImplicitFrequency;
				in >> *dynamic_cast<mCaseImplicitFrequency*>(obj);
				break;
			case MxFunctions::ASCoupling:
				obj = new mCaseImplASCoupling;
				in >> *dynamic_cast<mCaseImplASCoupling*>(obj);
				break;
			case MxFunctions::ImplicitTransient:
				obj = new mCaseImplicitTransient;
				in >> *dynamic_cast<mCaseImplicitTransient*>(obj);
				break;
			case MxFunctions::ImplicitNonLinearDynamic:
				obj = new mCaseImplicitNonLinearDynamic;
				in >> *dynamic_cast<mCaseImplicitNonLinearDynamic*>(obj);
				break;
			case MxFunctions::ImplicitLinearBuckling:
				obj = new mCaseImplicitLinearBuckling;
				in >> *dynamic_cast<mCaseImplicitLinearBuckling*>(obj);
				break;
			case MxFunctions::ImplicitPostBuckling:
				obj = new mCaseImplicitNonLinearArc;
				in >> *dynamic_cast<mCaseImplicitNonLinearArc*>(obj);
				break;
			case MxFunctions::ImplicitSteadyHeat:
				obj = new mCaseImplicitSteadyHeat;
				in >> *dynamic_cast<mCaseImplicitSteadyHeat*>(obj);
				break;
			case MxFunctions::ImplicitTransientHeat:
				obj = new mCaseImplicitTransientHeat;
				in >> *dynamic_cast<mCaseImplicitTransientHeat*>(obj);
				break;
			case MxFunctions::ImplicitStructuralCouplingHeat:
				obj = new mCaseImplicitStructuralCouplingHeat;
				in >> *dynamic_cast<mCaseImplicitStructuralCouplingHeat*>(obj);
				break;
			case MxFunctions::ImplicitGeneric:
				obj = new mCaseImplicitGeneric;
				in >> *dynamic_cast<mCaseImplicitGeneric*>(obj);
				break;
			default:
				break;
			}

			if (obj)
				mgr.AddCaseData(obj);
		}
		return in;
	}


	inline QDataStream& operator>>(QDataStream& in, mCaseBase& obj)
	{
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammCaseBaseV3(in, obj);
	    }
	    else
	    {
	        InStreammCaseBaseV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammCaseBaseV3(QDataStream& in, mCaseBase& obj)
	{
		qint32 val;
		in >> val;
		obj._caseType = static_cast<MxFunctions::CaseType>(val);
		in >> obj._id >> obj._caseName >> obj._solveDir >> obj._caseDescribe >> obj._outputs;
		in >> obj._historyOutputs >> obj._historyKdOutputs >> obj._historyOutputEnabled;
		in >> obj._hstFileType >> obj._hstDataType >> obj._hstDynamicStiff;
		in >> obj._outputType >> obj._outputs >> obj._selectedset;
		in >> obj._pch_outputs >> obj._pch_selectedset;
		return in;
	}


	inline QDataStream& operator>>(QDataStream& in, mCaseImplicitLinearStatic& obj)
	{
	    in >> static_cast<mCaseBase&>(obj);
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammCaseImplicitLinearStaticV3(in, obj);
	    }
	    else
	    {
	        InStreammCaseImplicitLinearStaticV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammCaseImplicitLinearStaticV3(QDataStream& in, mCaseImplicitLinearStatic& obj)
	{
		in >> obj._equationSolver >> obj._inertiaRelief >> obj._autospc;
		return in;
	}


	inline QDataStream& operator>>(QDataStream& in, mCaseImplicitNonLinearStatic& obj)
	{
	    in >> static_cast<mCaseBase&>(obj);
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammCaseImplicitNonLinearStaticV3(in, obj);
	    }
	    else
	    {
	        InStreammCaseImplicitNonLinearStaticV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammCaseImplicitNonLinearStaticV3(QDataStream& in, mCaseImplicitNonLinearStatic& obj)
	{
		in >> obj._nonLinearity >> obj._cntnlsub >> obj._inertiaRelief >> obj._Sparsematrix >> obj._timeStepType >> obj._initialStepValue;
		in >> obj._totolTimeValue >> obj._minTimeStep >> obj._maxTimeStep >> obj._visco >> obj._cetol;
		return in;
	}


    inline QDataStream& operator>>(QDataStream& in, mCaseImplicitModal& obj)
    {
        in >> static_cast<mCaseBase&>(obj);
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammCaseImplicitModalV3(in, obj);
        }
        else
        {
            InStreammCaseImplicitModalV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammCaseImplicitModalV3(QDataStream& in, mCaseImplicitModal& obj)
    {
        in >> obj._modalType >> obj._modalSolver >> obj._nlgeo >> obj._complexMethod >> obj._minFrequency;
        in >> obj._maxFrequency >> obj._order >> obj._tolerance >> obj._ampffact >> obj._normalization >> obj._rootNum >> obj._dampingG;
        in >> obj._standCoor >> obj._picChecked >> obj._picValue;
        in >> obj._rotationalChecked;
        return in;
    }



	inline QDataStream& operator>>(QDataStream& in, mCaseImplicitResponseSpectrum& obj)
	{
	    in >> static_cast<mCaseBase&>(obj);
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammCaseImplicitResponseSpectrumV3(in, obj);
	    }
	    else
	    {
	        InStreammCaseImplicitResponseSpectrumV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammCaseImplicitResponseSpectrumV3(QDataStream& in, mCaseImplicitResponseSpectrum& obj)
	{
		in >> obj._modalSolver >> obj._minFrequency >> obj._maxFrequency >> obj._order >> obj._tolerance >> obj._ampffact;
		in >> obj._normalization >> obj._dampingG >> obj._dampingAlpha1 >> obj._dampingAlpha2;
		in >> obj._dirction >> obj._modal >> obj._modalFrequencyClose >> obj._tableData;
		return in;
	}


	inline QDataStream& operator>>(QDataStream& in, mCaseImplicitRandomVibration& obj)
	{
	    in >> static_cast<mCaseBase&>(obj);
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammCaseImplicitRandomVibrationV3(in, obj);
	    }
	    else
	    {
	        InStreammCaseImplicitRandomVibrationV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammCaseImplicitRandomVibrationV3(QDataStream& in, mCaseImplicitRandomVibration& obj)
	{
		in >> obj._time0 >> obj._timeMax >> obj._nTime >> obj._tableXYData >> obj.m_outputData;
		return in;
	}


	inline QDataStream& operator>>(QDataStream& in, mCaseImplicitFrequency& obj)
	{
	    in >> static_cast<mCaseBase&>(obj);
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammCaseImplicitFrequencyV3(in, obj);
	    }
	    else
	    {
	        InStreammCaseImplicitFrequencyV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammCaseImplicitFrequencyV3(QDataStream& in, mCaseImplicitFrequency& obj)
	{
		in >> obj._method >> obj._isRemnantOpen >> obj._modalSolver >> obj._minFrequency >> obj._maxFrequency >> obj._order >> obj._tolerance;
		in >> obj._ampffact >> obj._normalization;
		in >> obj._dampingG >> obj._dampingAlpha1 >> obj._dampingAlpha2;
		in >> obj.checked;

		bool hasModalContriData;
		in >> hasModalContriData;
		if (hasModalContriData)
		{
			obj._modalContriData = new mModalContriData();
			in >> *(obj._modalContriData);
		}

		return in;
	}


	inline QDataStream& operator>>(QDataStream& in, mCaseImplASCoupling& obj)
	{
	    in >> static_cast<mCaseBase&>(obj);
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammCaseImplASCouplingV3(in, obj);
	    }
	    else
	    {
	        InStreammCaseImplASCouplingV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammCaseImplASCouplingV3(QDataStream& in, mCaseImplASCoupling& obj)
	{
		in >> obj._method >> obj._isRemnantOpen >> obj._modalSolver >> obj._minFrequency >> obj._maxFrequency >> obj._order >> obj._tolerance;
		in >> obj._ampffact >> obj._normalization
			>> obj._structMinFrequency >> obj._structMaxFrequency >> obj._structOrder >> obj._structTolerance
			>> obj._structAmpffact >> obj._structNormalization
			>> obj._acousticsMinFrequency >> obj._acousticsMaxFrequency >> obj._acousticsOrder >> obj._acousticsTolerance
			>> obj._acousticsAmpffact >> obj._acousticsNormalization
			;
		in >> obj._dampingG >> obj._dampingAlpha1 >> obj._dampingAlpha2
			>> obj._structDampingG >> obj._structDampingAlpha1 >> obj._structDampingAlpha2
			>> obj._acousticsDampingG >> obj._acousticsDampingAlpha1 >> obj._acousticsDampingAlpha2;
		in >> obj.checked;

		bool hasmodalContriData;
		in >> hasmodalContriData;
		if (hasmodalContriData)
		{
			obj._modalContriData = new mModalContriData();
			in >> *(obj._modalContriData);
		}

		bool hasNodeContriData;
		in >> hasNodeContriData;
		if (hasNodeContriData)
		{
			obj._mNodeContriData = new mNodeContriData();
			in >> *(obj._mNodeContriData);
		}

		bool hasPanelContriData;
		in >> hasPanelContriData;
		if (hasPanelContriData)
		{
			obj._mPanelContriData = new mPanelContriData();
			in >> *(obj._mPanelContriData);
		}

		return in;
	}


	inline QDataStream& operator>>(QDataStream& in, mCaseImplicitTransient& obj)
	{
	    in >> static_cast<mCaseBase&>(obj);
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammCaseImplicitTransientV3(in, obj);
	    }
	    else
	    {
	        InStreammCaseImplicitTransientV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammCaseImplicitTransientV3(QDataStream& in, mCaseImplicitTransient& obj)
	{
		in >> obj._method >> obj._solveMethod >> obj._isRemnantOpen >> obj._timestepCount >> obj._timestepVal >> obj._modalSolver >> obj._minFrequency;
		in >> obj._maxFrequency >> obj._order >> obj._tolerance >> obj._ampffact >> obj._normalization >> obj._dampingG >> obj._dampingOmg3 >> obj._dampingAlpha1;
		in >> obj._dampingAlpha2;
		return in;
	}


	inline QDataStream& operator>>(QDataStream& in, mCaseImplicitNonLinearDynamic& obj)
	{
	    in >> static_cast<mCaseBase&>(obj);
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammCaseImplicitNonLinearDynamicV3(in, obj);
	    }
	    else
	    {
	        InStreammCaseImplicitNonLinearDynamicV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammCaseImplicitNonLinearDynamicV3(QDataStream& in, mCaseImplicitNonLinearDynamic& obj)
	{
		in >> obj._nonLinearity >> obj._cntnlsub >> obj._inertiaRelief >> obj._timeStepType >> obj._initialStepValue;
		in >> obj._totolTimeValue >> obj._minTimeStep >> obj._maxTimeStep >> obj._quasiStaticAnasys;
		return in;
	}


	inline QDataStream& operator>>(QDataStream& in, mCaseImplicitLinearBuckling& obj)
	{
	    in >> static_cast<mCaseBase&>(obj);
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammCaseImplicitLinearBucklingV3(in, obj);
	    }
	    else
	    {
	        InStreammCaseImplicitLinearBucklingV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammCaseImplicitLinearBucklingV3(QDataStream& in, mCaseImplicitLinearBuckling& obj)
	{
		in >> obj._minFrequency >> obj._maxFrequency >> obj._order >> obj._tolerance;
		return in;
	}


	inline QDataStream& operator>>(QDataStream& in, mCaseImplicitNonLinearArc& obj)
	{
	    in >> static_cast<mCaseBase&>(obj);
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammCaseImplicitNonLinearArcV3(in, obj);
	    }
	    else
	    {
	        InStreammCaseImplicitNonLinearArcV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammCaseImplicitNonLinearArcV3(QDataStream& in, mCaseImplicitNonLinearArc& obj)
	{
		in >> obj._nonLinearity >> obj._cntnlsub >> obj._inertiaRelief >> obj._timeStepType >> obj._initialStepValue;
		in >> obj._totolTimeValue >> obj._minTimeStep >> obj._maxTimeStep >> obj._lMax >> obj._nMax;
		in >> obj._initBug;
		qint32 bugType;
		in >> bugType;
		obj._initBugType = static_cast<MxFunctions::InitBugType>(bugType);
		in >> obj._initBugFileName >> obj._scaleValue;
		return in;
	}


	inline QDataStream& operator>>(QDataStream& in, mCaseImplicitSteadyHeat& obj)
	{
	    in >> static_cast<mCaseBase&>(obj);
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammCaseImplicitSteadyHeatV3(in, obj);
	    }
	    else
	    {
	        InStreammCaseImplicitSteadyHeatV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammCaseImplicitSteadyHeatV3(QDataStream& in, mCaseImplicitSteadyHeat& obj)
	{
		in >> obj._cntnlsub >> obj._timeStepType >> obj._initialStepValue >> obj._totolTimeValue >> obj._minTimeStep >> obj._maxTimeStep;
		return in;
	}


	inline QDataStream& operator>>(QDataStream& in, mCaseImplicitTransientHeat& obj)
	{
	    in >> static_cast<mCaseBase&>(obj);
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammCaseImplicitTransientHeatV3(in, obj);
	    }
	    else
	    {
	        InStreammCaseImplicitTransientHeatV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammCaseImplicitTransientHeatV3(QDataStream& in, mCaseImplicitTransientHeat& obj)
	{
		in >> obj._cntnlsub >> obj._timeStepType >> obj._initialStepValue >> obj._totolTimeValue >> obj._minTimeStep >> obj._maxTimeStep;
		return in;
	}


	inline QDataStream& operator>>(QDataStream& in, mCaseImplicitGeneric& obj)
	{
	    in >> static_cast<mCaseBase&>(obj);
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammCaseImplicitGenericV3(in, obj);
	    }
	    else
	    {
	        InStreammCaseImplicitGenericV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammCaseImplicitGenericV3(QDataStream& in, mCaseImplicitGeneric& obj)
	{
		return in;
	}


	inline QDataStream& operator>>(QDataStream& in, mCaseImplicitStructuralCouplingHeat& obj)
	{
	    in >> static_cast<mCaseBase&>(obj);
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammCaseImplicitStructuralCouplingHeatV3(in, obj);
	    }
	    else
	    {
	        InStreammCaseImplicitStructuralCouplingHeatV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammCaseImplicitStructuralCouplingHeatV3(QDataStream& in, mCaseImplicitStructuralCouplingHeat& obj)
	{
		in >> obj._cntnlsub >> obj._timeStepType >> obj._initialStepValue >> obj._totolTimeValue >> obj._minTimeStep >> obj._maxTimeStep;
		return in;
	}

}
