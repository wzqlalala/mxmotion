#include "mOptimizationGoalData.h"

namespace MechStructOptionData
{
	mOptimizationGoalData::mOptimizationGoalData()
	{

		_objectiveType = QString("最小化");
		_responseType = QString("柔度");
		_node = { MxFunctions::PickDataType::None , {} };
		_isCompliance = false;
		_isFrequency = false;
		_nodeDofs = "1";
		_frequencyTargetType = "指定频率点";
		_specifiedFrequency = "";
		_frfValueType = "幅值";
	}

	mOptimizationGoalData::~mOptimizationGoalData()
	{


	}
	MxFunctions::BrowserTreeStatus mOptimizationGoalData::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		QString tip = "警告";
		if ((_responseType == "位移" || _responseType == "PSD位移" || _responseType == "频响位移") && _node.first == MxFunctions::PickDataType::None)
		{
			info.append({ tip,"节点未选择" });
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
		}
		else if (_responseType == "柔度指数" && !(_isCompliance || _isFrequency))
		{
			info.append({ tip,"至少选择柔度或模态其中之一" });
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
		}

		return status;
	}
}

