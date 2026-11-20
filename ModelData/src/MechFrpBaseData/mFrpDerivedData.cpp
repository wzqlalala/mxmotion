#include "mFrpDerivedData.h"

namespace MechFrpData
{

	mFrpAnalysisData::mFrpAnalysisData()
		: mFrpBaseData(MxFunctions::FrpNode_Analysis1)
	{

	}

	mFrpBaseData* mFrpAnalysisData::clone(mFrpBaseData* parentNode) const
	{
		mFrpAnalysisData* copy = new mFrpAnalysisData(*this);
		copy->m_pParent = parentNode;
		cloneChildrenTo(copy);
		return copy;
	}

	mFrpMaterialData::mFrpMaterialData()
		: mFrpBaseData(MxFunctions::FrpNode_Material)
	{
	}

	mFrpBaseData* mFrpMaterialData::clone(mFrpBaseData* parentNode) const
	{
		mFrpMaterialData* copy = new mFrpMaterialData(*this);
		copy->m_pParent = parentNode;
		cloneChildrenTo(copy);
		return copy;
	}

	mFrpPhaseData::mFrpPhaseData(FrpNodeType type)
		: mFrpBaseData(type)
	{
	}

	mFrpBaseData* mFrpPhaseData::clone(mFrpBaseData* parentNode) const
	{
		mFrpPhaseData* copy = new mFrpPhaseData(*this);
		copy->m_pParent = parentNode;
		cloneChildrenTo(copy);
		return copy;
	}

	mFrpRveData::mFrpRveData()
		: mFrpBaseData(MxFunctions::FrpNode_Rve)
	{
	}

	mFrpBaseData* mFrpRveData::clone(mFrpBaseData* parentNode) const
	{
		mFrpRveData* copy = new mFrpRveData(*this);
		copy->m_pParent = parentNode;
		cloneChildrenTo(copy);
		return copy;
	}

	mFrpFailureData::mFrpFailureData()
		: mFrpBaseData(MxFunctions::FrpNode_Failure)
	{
	}

	mFrpBaseData* mFrpFailureData::clone(mFrpBaseData* parentNode) const
	{
		mFrpFailureData* copy = new mFrpFailureData(*this);
		copy->m_pParent = parentNode;
		cloneChildrenTo(copy);
		return copy;
	}

	mFrpAspectRatioData::mFrpAspectRatioData()
		: mFrpBaseData(MxFunctions::FrpNode_AspectRatio)
	{
	}

	mFrpBaseData* mFrpAspectRatioData::clone(mFrpBaseData* parentNode) const
	{
		mFrpAspectRatioData* copy = new mFrpAspectRatioData(*this);
		copy->m_pParent = parentNode;
		cloneChildrenTo(copy);
		return copy;
	}

}
