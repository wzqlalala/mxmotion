#include "mOptimizationDesignData.h"
#include "mNodeValidor.h"

namespace MechStructOptionData
{
	mOptimizationDesignData::mOptimizationDesignData(int id, QString name)
	{
		_id = id;
		_name = name;

		_designDomainType = "实体";
		_optimizationMethod = "厚度";
		_baseThickness = "0.0";
		_ribHeightStatus = false;
		_ribWidthStatus = false;
		_ribHeight = "";
		_ribWidth = "";

		_minMemberStatus = false;
		_maxMemberStatus = false;
		_minGapStatus = false;
		_stressConstraintStatus = false;

		_patternStatus = false;
		_patternType = "单平面";
		_liftingStatus = false;
		_liftingType = "单向";
		_nohole = "关闭";
		_extrusionStatus = false;
		_extrusionType = "平行";
		_supportStatus = false;
		_patternRepStatus = false;
		_domainType = "主设计域";
		_coordType = "全局坐标系";

		_originId = -1;
		_dirPoint1Id = -1;
		_dirPoint2Id = -1;
		_startPointId = -1;
		_dirPointId = -1;
		_supportStartPointId = -1;
		_supportDirPointId = -1;
		_repBasePointId = -1;
		_repDirPoint1Id = -1;
		_repDirPoint2Id = -1;
		_repDirPoint3Id = -1;

		_morphBoundStatus = false;
		_morphOriginId = -1;
		_morphDirPoint1Id = -1;
		_morphDirPoint2Id = -1;

		_shapeSplitDomain = false;
		_shapeSplitPart = false;
		_shapeKeepHandle = false;
		_shape2DSplit = false;
		_shapeDeformMode = "交互";
		_shapeTx = "";
		_shapeTy = "";
		_shapeTz = "";
		_shapeRotAngle = "";
		_shapeRotAxis = "X轴";
		_shapeCustomAxis = "";
		_shapeTargetNode = "";
		_shapeInitValue = "";
		_shapeMinValue = "";
		_shapeMaxValue = "";
		_shapeMoveLimit = "";
	}

	mOptimizationDesignData::~mOptimizationDesignData()
	{


	}
	void mOptimizationDesignData::setId(int id)
	{
		_id = id;
	}

	int mOptimizationDesignData::getId()
	{
		return _id;
	}

	void mOptimizationDesignData::setName(const QString& name)
	{
		_name = name;
	}

	QString mOptimizationDesignData::getName()
	{
		return _name;
	}

	void mOptimizationDesignData::setDescribe(const QString& describe)
	{
		_describe = describe;
	}

	QString mOptimizationDesignData::getDescribe()
	{
		return _describe;
	}

	mOptimizationDesignData* mOptimizationDesignData::copyData(const int& id, const QString& name)
	{
		mOptimizationDesignData* data = new mOptimizationDesignData(id, name);
		data->_designDomainType = _designDomainType;
		data->_optimizationMethod = _optimizationMethod;
		data->_baseThickness = _baseThickness;
		data->_ribHeightStatus = _ribHeightStatus;
		data->_ribWidthStatus = _ribWidthStatus;
		data->_ribHeight = _ribHeight;
		data->_ribWidth = _ribWidth;
		data->_minMemberStatus = _minMemberStatus;
		data->_minMemberSize = _minMemberSize;
		data->_maxMemberStatus = _maxMemberStatus;
		data->_maxMemberSize = _maxMemberSize;
		data->_minGapStatus = _minGapStatus;
		data->_minGapSize = _minGapSize;
		data->_stressConstraintStatus = _stressConstraintStatus;
		data->_stressConstraintValue = _stressConstraintValue;
		data->_patternStatus = _patternStatus;
		data->_patternType = _patternType;
		data->_patternNum = _patternNum;
		data->_origin = _origin;
		data->_dirPoint1 = _dirPoint1;
		data->_dirPoint2 = _dirPoint2;
		data->_liftingStatus = _liftingStatus;
		data->_liftingType = _liftingType;
		data->_nohole = _nohole;
		data->_startPoint = _startPoint;
		data->_dirPoint = _dirPoint;
		data->_extrusionStatus = _extrusionStatus;
		data->_extrusionType = _extrusionType;
		data->_path1 = _path1;
		data->_path2 = _path2;
		data->_supportStatus = _supportStatus;
		data->_overhangAngle = _overhangAngle;
		data->_supportStartPoint = _supportStartPoint;
		data->_supportDirPoint = _supportDirPoint;
		data->_patternRepStatus = _patternRepStatus;
		data->_domainType = _domainType;
		data->_scaleX = _scaleX;
		data->_scaleY = _scaleY;
		data->_scaleZ = _scaleZ;
		data->_morphUpLimit = _morphUpLimit;
		data->_morphDownLimit = _morphDownLimit;
		data->_morphInitRatio = _morphInitRatio;
		data->_morphMode = _morphMode;
		data->_morphMinWidth = _morphMinWidth;
		data->_morphAngle = _morphAngle;
		data->_morphHeight = _morphHeight;
		data->_morphDirection = _morphDirection;
		data->_morphBoundStatus = _morphBoundStatus;
		data->_morphOrigin = _morphOrigin;
		data->_morphDirPoint1 = _morphDirPoint1;
		data->_morphDirPoint2 = _morphDirPoint2;

		data->_shapeSplitDomain = _shapeSplitDomain;
		data->_shapeSplitPart = _shapeSplitPart;
		data->_shapeKeepHandle = _shapeKeepHandle;
		data->_shape2DSplit = _shape2DSplit;
		data->_shapeDeformMode = _shapeDeformMode;
		data->_shapeTx = _shapeTx;
		data->_shapeTy = _shapeTy;
		data->_shapeTz = _shapeTz;
		data->_shapeRotAngle = _shapeRotAngle;
		data->_shapeRotPick = _shapeRotPick;
		data->_shapeRotAxis = _shapeRotAxis;
		data->_shapeCustomAxis = _shapeCustomAxis;
		data->_shapeTargetNode = _shapeTargetNode;
		data->_shapeInitValue = _shapeInitValue;
		data->_shapeMinValue = _shapeMinValue;
		data->_shapeMaxValue = _shapeMaxValue;
		data->_shapeMoveLimit = _shapeMoveLimit;
		return data;
	}

	MxFunctions::BrowserTreeStatus mOptimizationDesignData::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		if (_minMemberStatus)
		{
			checkRequiredPositive(_minMemberSize, "最小成员尺寸", info, status);
		}
		if (_maxMemberStatus)
		{
			checkOptionalPositive(_maxMemberSize, "最大成员尺寸", info, status);
		}
		if (_minGapStatus)
		{
			checkOptionalPositive(_minGapSize, "最小间隙", info, status);
		}
		if (_stressConstraintStatus)
		{
			checkOptionalPositive(_stressConstraintValue, "应力约束", info, status);
		}
		if (_supportStatus)
		{
			checkRequiredReal(_overhangAngle, "悬垂角", info, status);
		}
		if (_patternRepStatus)
		{
			checkOptionalPositive(_scaleX, "缩放系数X", info, status);
			checkOptionalPositive(_scaleY, "缩放系数Y", info, status);
			checkOptionalPositive(_scaleZ, "缩放系数Z", info, status);
		}
		return status;
	}

}
