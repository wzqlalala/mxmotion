#include "mOptimizationConstraintData.h"

namespace MechStructOptionData
{
	mOptimizationConstraintData::mOptimizationConstraintData(int id, QString name)
	{
		_id = id;
		_name = name;

		_upperBoundStatus = false;
		_lowerBoundStatus = false;
		_isArea = false;

		_constraintType = "质量";

		//响应类型参数默认值(与优化目标保持一致)
		_frequencyOrder = "";
		_isCompliance = false;
		_isFrequency = false;
		_node = { MxFunctions::PickDataType::None , {} };
		_nodeDofs = "1";
		_frequencyTargetType = "指定频率点";
		_specifiedFrequency = "";
		_frfValueType = "幅值";
	}

	mOptimizationConstraintData::~mOptimizationConstraintData()
	{


	}
	void mOptimizationConstraintData::setId(int id)
	{
		_id = id;
	}

	int mOptimizationConstraintData::getId()
	{
		return _id;
	}

	void mOptimizationConstraintData::setName(const QString& name)
	{
		_name = name;
	}

	QString mOptimizationConstraintData::getName()
	{
		return _name;
	}

	void mOptimizationConstraintData::setDescribe(const QString& describe)
	{
		_describe = describe;
	}

	QString mOptimizationConstraintData::getDescribe()
	{
		return _describe;
	}

	MxFunctions::BrowserTreeStatus mOptimizationConstraintData::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		QString tip = "警告";
		if ((_constraintType == "位移" || _constraintType == "PSD位移" || _constraintType == "频响位移") && _node.first == MxFunctions::PickDataType::None)
		{
			info.append({ tip,"节点未选择" });
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
		}
		else if (_constraintType == "柔度指数" && !(_isCompliance || _isFrequency))
		{
			info.append({ tip,"至少选择柔度或模态其中之一" });
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
		}

		return status;
	}

	mOptimizationConstraintData* mOptimizationConstraintData::copyData(const int& id, const QString& name)
	{
		mOptimizationConstraintData* data = new mOptimizationConstraintData(id, name);
		data->_upperBoundStatus = _upperBoundStatus;
		data->_lowerBoundStatus = _lowerBoundStatus;
		data->_isArea = _isArea;
		data->_constraintType = _constraintType;
		data->_frequencyOrder = _frequencyOrder;
		data->_isCompliance = _isCompliance;
		data->_isFrequency = _isFrequency;
		data->_caseToweight = _caseToweight;
		data->_order = _order;
		data->_node = _node;
		data->_nodeDofs = _nodeDofs;
		data->_frequencyTargetType = _frequencyTargetType;
		data->_specifiedFrequency = _specifiedFrequency;
		data->_frfValueType = _frfValueType;
		return data;
	}

}

