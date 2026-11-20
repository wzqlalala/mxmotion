#include "mPlyLaminate.h"

using namespace MxFunctions;
namespace MechPropertyData
{
	mPlyLaminate::mPlyLaminate(int id, PropertyType type, const QString& name) 
		: mPropertyBase(id, type, name)
	{
		/**************************************共享变量***********************************************/
		_type = 0;
		_plyDefType = 0;
		_materialDir = 0;
		_normaldirection = 3;
		_rotAngle = "";
		thicknessList = {};
		angleList = {};
		intePointList = {};
		/**************************************壳变量***********************************************/
		_shellProperty = "无";
		_referenceSurfacePos = "无";
		_referenceSurfaceDis = "0";
		_failure = "无";
		_thickness = "截面厚度";
		/**************************************实体壳变量*********************************************/
		_solidShellProperty = "无";
		/**************************************实体变量***********************************************/
		_solidProperty = "无";
		eleMentDir = 3;

		_rotAngleType = 0;
	}

	mPlyLaminate::~mPlyLaminate()
	{

	}

	mPlyLaminate* MechPropertyData::mPlyLaminate::copyProperty(const int& id, const QString& name)
	{
		mPlyLaminate* caseData = new mPlyLaminate(id, getPropertyType(), name);
		caseData->_type = _type;
		caseData->_plyDefType = _plyDefType;
		caseData->_materialDir = _materialDir;
		caseData->_normaldirection = _normaldirection;
		caseData->_rotAngle = _rotAngle;
		caseData->thicknessList = thicknessList;
		caseData->angleList = angleList;
		caseData->intePointList = intePointList;
		caseData->_shellProperty = _shellProperty;
		caseData->_referenceSurfacePos = _referenceSurfacePos;
		caseData->_referenceSurfaceDis = _referenceSurfaceDis;
		caseData->_failure = _failure;
		caseData->_thickness = _thickness;
		caseData->_solidShellProperty = _solidShellProperty;
		caseData->_solidProperty = _solidProperty;
		caseData->eleMentDir = eleMentDir;
		return caseData;
	}
}
