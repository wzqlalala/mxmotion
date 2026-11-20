#include "mRunnerOptimizationDesignData.h"

#include "mNodeValidor.h"

namespace MechStructOptionData
{
	mRunnerOptimizationDesignData::mRunnerOptimizationDesignData(int id, QString name)
	{
		_id = id;
		_name = name;

		_designDomainType = "实体";

		_maxPermeabilityStatus = false;
		_minPermeabilityStatus = false;
		_maxPermeability = "";
		_minPermeability = "";

		_minMemberStatus = false;
		_maxMemberStatus = false;
		_minGapStatus = false;

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
	}

	mRunnerOptimizationDesignData::~mRunnerOptimizationDesignData()
	{


	}
	void mRunnerOptimizationDesignData::setId(int id)
	{
		_id = id;
	}

	int mRunnerOptimizationDesignData::getId()
	{
		return _id;
	}

	void mRunnerOptimizationDesignData::setName(const QString& name)
	{
		_name = name;
	}

	QString mRunnerOptimizationDesignData::getName()
	{
		return _name;
	}

	void mRunnerOptimizationDesignData::setDescribe(const QString& describe)
	{
		_describe = describe;
	}

	QString mRunnerOptimizationDesignData::getDescribe()
	{
		return _describe;
	}

	mRunnerOptimizationDesignData* mRunnerOptimizationDesignData::copyData(const int& id, const QString& name)
	{
		mRunnerOptimizationDesignData* data = new mRunnerOptimizationDesignData(id, name);
		data->_designDomainType = _designDomainType;

		data->_maxPermeabilityStatus = _maxPermeabilityStatus;
		data->_minPermeabilityStatus = _minPermeabilityStatus;
		data->_maxPermeability = _maxPermeability;
		data->_minPermeability = _minPermeability;

		data->_minMemberStatus = _minMemberStatus;
		data->_minMemberSize = _minMemberSize;
		data->_maxMemberStatus = _maxMemberStatus;
		data->_maxMemberSize = _maxMemberSize;
		data->_minGapStatus = _minGapStatus;
		data->_minGapSize = _minGapSize;
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
		return data;
	}

	MxFunctions::BrowserTreeStatus mRunnerOptimizationDesignData::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		QString tip = "警告";
		checkRequiredPositive(_minMemberSize, "最小成员尺寸", info, status);
		checkOptionalPositive(_maxMemberSize, "最大成员尺寸", info, status);
		checkOptionalPositive(_minGapSize, "最小间隙", info, status);
		return status;
	}

}
