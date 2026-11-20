#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mSetDataInStream.h"
#include "mSetDataOutStream.h"
#include "mMaterialDataInStream.h"
#include "mMaterialDataOutStream.h"
#include "mMeshDataInStream.h"
#include "mMeshDataOutStream.h"
#include "mContactDataInStream.h"
#include "mContactDataOutStream.h"
#include "mPeriodicBoundaryInStream.h"
#include "mPeriodicBoundaryOutStream.h"
#include "mCoordinateInStream.h"
#include "mCoordinateOutStream.h"
#include "mConnectorInStream.h"
#include "mConnectorOutStream.h"
#include "mCaseDataInStream.h"
#include "mCaseDataOutStream.h"
#include "mLoadBoundaryInStream.h"
#include "mLoadBoundaryOutStream.h"
#include "mPredefinedFieldInStream.h"
#include "mPredefinedFieldOutStream.h"
#include "mMapRelationInStream.h"
#include "mMapRelationOutStream.h"
#include "mGeoDataIOStream.h"
#include "mModelData.h"
#include "mPropertyDataInStream.h"
#include "mPropertyDataOutStream.h"
#include "mSectionShapeInStream.h"
#include "mSectionShapeOutStream.h"
#include "mElementDataInStream.h"
#include "mElementDataOutStream.h"
#include "mStructOptionInStream.h"
#include "mStructOptionOutStream.h"
#include "mRunnerStructOptionInStream.h"
#include "mRunnerStructOptionOutStream.h"
#include "mFreqSamplPointInStream.h"
#include "mFreqSamplPointOutStream.h"
#include "mRotateAnalyseInStream.h"
#include "mRotateAnalyseOutStream.h"
#include "mLaminateDataInStream.h"
#include "mLaminateDataOutStream.h"
#include "mGlobalParamInStream.h"
#include "mGlobalParamOutStream.h"
#include "mNonStructuralQualityDataInStream.h"
#include "mNonStructuralQualityDataOutStream.h"
#include "mFunctionDataInStream.h"
#include "mFunctionDataOutStream.h"
#include "mDiscreteFieldDataInStream.h"
#include "mDiscreteFieldDataOutStream.h"
#include "mDeputyInStream.h"
#include "mDeputyOutStream.h"
#include "mMeshPanelDataInStream.h"
#include "mMeshPanelDataOutStream.h"
#include "mModelFixInStream.h"
#include "mModelFixOutStream.h"
#include "mFrpDataInStream.h"
#include "mFrpDataOutStream.h"
#include "mMotionDataInStream.h"
#include "mMotionDataOutStream.h"

namespace MechModelData
{

	inline QDataStream& operator>>(QDataStream& in, mModelData& obj)
	{
		//版本号高先判断，兼容所有低版本与高版本
		if (FILE_VERSION >= 0x3000)
		{
			InStreammModelDataV3(in, obj);
		}
		else
		{
			InStreammModelDataV3(in, obj);
		}
		return in;
	}

	inline QDataStream& InStreammModelDataV3(QDataStream& in, mModelData& obj)
	{
		in >> obj._modelDataName;
		in >> *obj._geoDataBase;
		in >> *obj._meshData;
		in >> *obj._elementData;
		in >> obj._caseManager;
		in >> obj._loadboundaryManager;
		in >> obj._rotateAnalyseManager;
		in >> obj._predefinedFieldManager;
		in >> obj._materialManager;
		in >> obj._propertyManager;
		in >> obj._sectionShapeManager;
		in >> obj._setManager;
		in >> obj._functionManager;
		in >> obj._contactManager;
		in >> obj._periodicBoundaryManager;
		in >> obj._connectorManager;
		in >> obj._coordinateManager;
		in >> obj._mapRelationManager;
		in >> obj._FreqSamplPointManager;
		in >> obj._LaminateManager;
		in >> *obj._mGlobalParam;
		in >> obj._nonStructuralQualityManager;
		in >> obj._fieldManager;
		in >> obj._deputyManager;
		in >> obj._modelFixManager;
		in >> obj._mFrpManager;

		bool hasStructOption;
		in >> hasStructOption;
		if (hasStructOption)
		{
			obj.createStructOption();
			in >> *obj._mStructOptionData;
		}

		bool hasRunnerStructOption;
		in >> hasRunnerStructOption;
		if (hasRunnerStructOption)
		{
			obj.createRunnerStructOption();
			in >> *obj._mRunnerStructOptionData;
		}

		//网格剖分/网格操作面板（旧版本文件无此数据，读到文件尾保持默认）
		in >> obj._meshPanelManager;
		in >> obj._meshOptPanelManager;
		in >> *obj._unitSystemManager;

		//【老类退役·S7】旧世界 set 桥回 GN + 引擎几何段恢复（cpp 实现）
		//（产品分支 _modelDataName 在流头读写，引擎段仍保持流真尾部以保 atEnd 判别）
		obj.adoptLegacyGeometry();
		obj.loadEngineGeometrySection(in);
		//装配层级段（旧版本文件无此段，atEnd 判别）
		obj.loadAssemblySection(in);

		//多体动力学（旧版本文件无此数据，读到文件尾保持默认）
		in >> obj._motionBodyManager;
		in >> obj._motionJointManager;
		in >> obj._motionCouplingManager;
		in >> obj._motionDriveManager;
		in >> obj._motionScenarioManager;
		in >> obj._motionMarkerManager;
		in >> obj._motionForceElementManager;
		return in;
	}

	inline QDataStream& operator<<(QDataStream& out, mModelData& obj)
	{
		out << obj._modelDataName;
		out << *obj._geoDataBase;
		out << *obj._meshData;
		out << *obj._elementData;
		out << obj._caseManager;
		out << obj._loadboundaryManager;
		out << obj._rotateAnalyseManager;
		out << obj._predefinedFieldManager;
		out << obj._materialManager;
		out << obj._propertyManager;
		out << obj._sectionShapeManager;
		out << obj._setManager;
		out << obj._functionManager;
		out << obj._contactManager;
		out << obj._periodicBoundaryManager;
		out << obj._connectorManager;
		out << obj._coordinateManager;
		out << obj._mapRelationManager;
		out << obj._FreqSamplPointManager;
		out << obj._LaminateManager;
		out << *obj._mGlobalParam;
		out << obj._nonStructuralQualityManager;
		out << obj._fieldManager;
		out << obj._deputyManager;
		out << obj._modelFixManager;
		out << obj._mFrpManager;

		if (obj._mStructOptionData)
		{
			out << true;
			out << *obj._mStructOptionData;
		}
		else
		{
			out << false;
		}

		if (obj._mRunnerStructOptionData)
		{
			out << true;
			out << *obj._mRunnerStructOptionData;
		}
		else
		{
			out << false;
		}

		out << obj._meshPanelManager;
		out << obj._meshOptPanelManager;
		out << *obj._unitSystemManager;
		//【老类退役·S7】引擎几何段嵌入（cpp 实现；旧版本读取端忽略尾部）
		obj.saveEngineGeometrySection(out);
		//装配层级段（追加在末尾写入；含空装配/空关系，保证空节点亦可读写还原）
		obj.saveAssemblySection(out);

		out << obj._motionBodyManager;
		out << obj._motionJointManager;
		out << obj._motionCouplingManager;
		out << obj._motionDriveManager;
		out << obj._motionScenarioManager;
		out << obj._motionMarkerManager;
		out << obj._motionForceElementManager;
		return out;
	}



}