#include "mPeakFilterData.h"

using namespace MxFunctions;
namespace MechGlobalParamData
{
	mPeakFilterData::mPeakFilterData(int id, QString name)
	{
		_id = id;
		_name = name;

		// 响应类型，默认=disp
		_respType = "disp";
		// 识别类型，默认=db
		_identType = "db";
		// 提取个数，默认=5
		_extractCount = "5";
		// 最小间距，默认=0.0
		_minSpacing = "0.0";
		// 最大间距，默认=0.0
		_maxSpacing = "0.0";
		// 起始频率，默认=0.0
		_startFreq = "0.0";
		// 终止频率，默认=0.0
		_endFreq = "0.0";
		// 应用到全局，默认=false
		_applyToGlobal = false;
		_tableData.append({ 0.0,0.0,0.0 });
	}

	mPeakFilterData::~mPeakFilterData()
	{


	}
	void mPeakFilterData::setId(int id)
	{
		_id = id;
	}

	int mPeakFilterData::getId()
	{
		return _id;
	}

	void mPeakFilterData::setName(const QString& name)
	{
		_name = name;
	}

	QString mPeakFilterData::getName()
	{
		return _name;
	}

	mPeakFilterData* mPeakFilterData::copyData(const int& id, const QString& name)
	{
		mPeakFilterData* data = new mPeakFilterData(id, name);
		data->_respType = _respType;
		data->_identType = _identType;
		data->_extractCount = _extractCount;
		data->_minSpacing = _minSpacing;
		data->_maxSpacing = _maxSpacing;
		data->_startFreq = _startFreq;
		data->_endFreq = _endFreq;
		data->_applyToGlobal = _applyToGlobal;
		data->_tableData = _tableData;
		return data;
	}

	MxFunctions::BrowserTreeStatus mPeakFilterData::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		return info.isEmpty() ? Tree_Yes : Tree_Failure;
	}

}
