#include "mModalContriData.h"
#include <QVector>
#include <QPair>
#include <QString>

using namespace MxFunctions;
namespace MechGlobalParamData
{
	mModalContriData::mModalContriData(int id, QString name)
	{
		_id = id;
		_name = name;

		//输出类型下拉框 structure，fluid
		_outPutType = "structure";
		//排序方式下拉框 mag，percentage
		_SortType = "mag";
		//贡献量结果类型 disp、vel、acc
		_ResultType = "disp";
		//过滤分数 默认0.001
		_fltScore = "0.001";
		//过滤分数默认勾选输出
		_fltScoreChecked = false;
		//输出阶数 默认3
		_OutOrd = "3";
		//输出阶数默认勾选输出
		_OutOrdChecked = false;
		//峰值筛选
		_checkBoxPkScr = false;
	}

	mModalContriData::~mModalContriData()
	{


	}
	void mModalContriData::setId(int id)
	{
		_id = id;
	}

	int mModalContriData::getId()
	{
		return _id;
	}

	void mModalContriData::setName(const QString& name)
	{
		_name = name;
	}

	QString mModalContriData::getName()
	{
		return _name;
	}

	mModalContriData* mModalContriData::copyData(const int& id, const QString& name)
	{
		mModalContriData* data = new mModalContriData(id, name);
		data->_outPutType = _outPutType;
		data->_SortType = _SortType;
		data->_ResultType = _ResultType;
		data->_fltScore = _fltScore;
		data->_fltScoreChecked = _fltScoreChecked;
		data->_OutOrd = _OutOrd;
		data->_OutOrdChecked = _OutOrdChecked;
		data->_checkBoxPkScr = _checkBoxPkScr;
		return data;
	}

	MxFunctions::BrowserTreeStatus mModalContriData::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		//if (_startID.isEmpty())
		//{
		//	info.append({ tip,"起始点编号为空" });
		//}
		return info.isEmpty() ? Tree_Yes : Tree_Failure;
	}

}
