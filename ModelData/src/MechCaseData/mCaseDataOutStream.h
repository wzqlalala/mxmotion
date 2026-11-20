#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include <QList>
#include <vector>

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
	inline QDataStream& operator<<(QDataStream& out, mCaseManager& mgr)
	{
		out << mgr._maxID;
		std::vector<int> caseIDs = mgr.getAllCaseID();
		int count = caseIDs.size();
		out << count;
		for (int i = 0; i < count; ++i) {
			mCaseBase* obj = mgr.getCaseData(caseIDs[i]);
			MxFunctions::CaseType type = obj->getCaseType();
			out << (qint32)type;
			switch (type)
			{
			case MxFunctions::ImplicitLinearStatic:
				out << *dynamic_cast<mCaseImplicitLinearStatic*>(obj);
				break;
			case MxFunctions::ImplicitNonLinearStatic:
				out << *dynamic_cast<mCaseImplicitNonLinearStatic*>(obj);
				break;
			case MxFunctions::ImplicitModal:
				out << *dynamic_cast<mCaseImplicitModal*>(obj);
				break;
			case MxFunctions::ImplicitResponseSpectrum:
				out << *dynamic_cast<mCaseImplicitResponseSpectrum*>(obj);
				break;
			case MxFunctions::ImplicitRandomVibration:
				out << *dynamic_cast<mCaseImplicitRandomVibration*>(obj);
				break;
			case MxFunctions::ImplicitFrequency:
				out << *dynamic_cast<mCaseImplicitFrequency*>(obj);
				break;
			case MxFunctions::ASCoupling:
				out << *dynamic_cast<mCaseImplASCoupling*>(obj);
				break;
			case MxFunctions::ImplicitTransient:
				out << *dynamic_cast<mCaseImplicitTransient*>(obj);
				break;
			case MxFunctions::ImplicitNonLinearDynamic:
				out << *dynamic_cast<mCaseImplicitNonLinearDynamic*>(obj);
				break;
			case MxFunctions::ImplicitLinearBuckling:
				out << *dynamic_cast<mCaseImplicitLinearBuckling*>(obj);
				break;
			case MxFunctions::ImplicitPostBuckling:
				out << *dynamic_cast<mCaseImplicitNonLinearArc*>(obj);
				break;
			case MxFunctions::ImplicitSteadyHeat:
				out << *dynamic_cast<mCaseImplicitSteadyHeat*>(obj);
				break;
			case MxFunctions::ImplicitTransientHeat:
				out << *dynamic_cast<mCaseImplicitTransientHeat*>(obj);
				break;
			case MxFunctions::ImplicitStructuralCouplingHeat:
				out << *dynamic_cast<mCaseImplicitStructuralCouplingHeat*>(obj);
				break;
			case MxFunctions::ImplicitGeneric:
				out << *dynamic_cast<mCaseImplicitGeneric*>(obj);
				break;
			default:
				break;
			}
		}
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, mCaseBase& obj)
	{
		qint32 val = static_cast<MxFunctions::CaseType>(obj._caseType);
		out << val;
		out << obj._id << obj._caseName << obj._solveDir << obj._caseDescribe << obj._outputs;
		out << obj._historyOutputs << obj._historyKdOutputs << obj._historyOutputEnabled;
		out << obj._hstFileType << obj._hstDataType << obj._hstDynamicStiff;
		out << obj._outputType << obj._outputs << obj._selectedset;
		out << obj._pch_outputs << obj._pch_selectedset;
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, mCaseImplicitLinearStatic& obj)
	{
		out << static_cast<mCaseBase&>(obj);
		out << obj._equationSolver << obj._inertiaRelief << obj._autospc;
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, mCaseImplicitNonLinearStatic& obj)
	{
		out << static_cast<mCaseBase&>(obj);
		out << obj._nonLinearity << obj._cntnlsub << obj._inertiaRelief << obj._Sparsematrix << obj._timeStepType << obj._initialStepValue;
		out << obj._totolTimeValue << obj._minTimeStep << obj._maxTimeStep << obj._visco << obj._cetol;
		return out;
	}

    inline QDataStream& operator<<(QDataStream& out, mCaseImplicitModal& obj)
    {
        out << static_cast<mCaseBase&>(obj);
        out << obj._modalType << obj._modalSolver << obj._nlgeo << obj._complexMethod << obj._minFrequency;
        out << obj._maxFrequency << obj._order << obj._tolerance << obj._ampffact << obj._normalization << obj._rootNum << obj._dampingG;
        out << obj._standCoor << obj._picChecked << obj._picValue;
        out << obj._rotationalChecked;
        return out;
    }

	inline QDataStream& operator<<(QDataStream& out, mCaseImplicitResponseSpectrum& obj)
	{
		out << static_cast<mCaseBase&>(obj);
		out << obj._modalSolver << obj._minFrequency << obj._maxFrequency << obj._order << obj._tolerance << obj._ampffact;
		out << obj._normalization << obj._dampingG << obj._dampingAlpha1 << obj._dampingAlpha2;
		out << obj._dirction << obj._modal << obj._modalFrequencyClose << obj._tableData;
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, mCaseImplicitRandomVibration& obj)
	{
		out << static_cast<mCaseBase&>(obj);
		out << obj._time0 << obj._timeMax << obj._nTime << obj._tableXYData << obj.m_outputData;
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, mCaseImplicitFrequency& obj)
	{
		out << static_cast<mCaseBase&>(obj);
		out << obj._method << obj._isRemnantOpen << obj._modalSolver << obj._minFrequency << obj._maxFrequency << obj._order << obj._tolerance;
		out << obj._ampffact << obj._normalization;
		out << obj._dampingG << obj._dampingAlpha1 << obj._dampingAlpha2;
		out << obj.checked;

		if (obj._modalContriData)
		{
			out << true;
			out << *(obj._modalContriData);
		}
		else
		{
			out << false;
		}

		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, mCaseImplASCoupling& obj)
	{
		out << static_cast<mCaseBase&>(obj);
		out << obj._method << obj._isRemnantOpen << obj._modalSolver
			<< obj._minFrequency << obj._maxFrequency << obj._order << obj._tolerance;
		out << obj._ampffact << obj._normalization
			<< obj._structMinFrequency << obj._structMaxFrequency << obj._structOrder << obj._structTolerance
			<< obj._structAmpffact << obj._structNormalization
			<< obj._acousticsMinFrequency << obj._acousticsMaxFrequency << obj._acousticsOrder << obj._acousticsTolerance
			<< obj._acousticsAmpffact << obj._acousticsNormalization
			;
		out << obj._dampingG << obj._dampingAlpha1 << obj._dampingAlpha2
			<< obj._structDampingG << obj._structDampingAlpha1 << obj._structDampingAlpha2
			<< obj._acousticsDampingG << obj._acousticsDampingAlpha1 << obj._acousticsDampingAlpha2;
		out << obj.checked;

		if (obj._modalContriData)
		{
			out << true;
			out << *(obj._modalContriData);
		}
		else
		{
			out << false;
		}

		if (obj._mNodeContriData)
		{
			out << true;
			out << *(obj._mNodeContriData);
		}
		else
		{
			out << false;
		}

		if (obj._mPanelContriData)
		{
			out << true;
			out << *(obj._mPanelContriData);
		}
		else
		{
			out << false;
		}

		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, mCaseImplicitTransient& obj)
	{
		out << static_cast<mCaseBase&>(obj);
		out << obj._method << obj._solveMethod << obj._isRemnantOpen << obj._timestepCount << obj._timestepVal << obj._modalSolver << obj._minFrequency;
		out << obj._maxFrequency << obj._order << obj._tolerance << obj._ampffact << obj._normalization << obj._dampingG << obj._dampingOmg3 << obj._dampingAlpha1;
		out << obj._dampingAlpha2;
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, mCaseImplicitNonLinearDynamic& obj)
	{
		out << static_cast<mCaseBase&>(obj);
		out << obj._nonLinearity << obj._cntnlsub << obj._inertiaRelief << obj._timeStepType << obj._initialStepValue;
		out << obj._totolTimeValue << obj._minTimeStep << obj._maxTimeStep << obj._quasiStaticAnasys;
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, mCaseImplicitLinearBuckling& obj)
	{
		out << static_cast<mCaseBase&>(obj);
		out << obj._minFrequency << obj._maxFrequency << obj._order << obj._tolerance;
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, mCaseImplicitNonLinearArc& obj)
	{
		out << static_cast<mCaseBase&>(obj);
		out << obj._nonLinearity << obj._cntnlsub << obj._inertiaRelief << obj._timeStepType << obj._initialStepValue;
		out << obj._totolTimeValue << obj._minTimeStep << obj._maxTimeStep << obj._lMax << obj._nMax;
		out << obj._initBug;
		out << static_cast<qint32>(obj._initBugType);
		out << obj._initBugFileName << obj._scaleValue;
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, mCaseImplicitSteadyHeat& obj)
	{
		out << static_cast<mCaseBase&>(obj);
		out << obj._cntnlsub << obj._timeStepType << obj._initialStepValue << obj._totolTimeValue << obj._minTimeStep << obj._maxTimeStep;
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, mCaseImplicitTransientHeat& obj)
	{
		out << static_cast<mCaseBase&>(obj);
		out << obj._cntnlsub << obj._timeStepType << obj._initialStepValue << obj._totolTimeValue << obj._minTimeStep << obj._maxTimeStep;
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, mCaseImplicitGeneric& obj)
	{
		out << static_cast<mCaseBase&>(obj);
		return out;
	}

    inline QDataStream& operator<<(QDataStream& out, mCaseImplicitStructuralCouplingHeat& obj)
    {
        out << static_cast<mCaseBase&>(obj);
        out << obj._cntnlsub << obj._timeStepType << obj._initialStepValue << obj._totolTimeValue << obj._minTimeStep << obj._maxTimeStep;
        return out;
    }
}
