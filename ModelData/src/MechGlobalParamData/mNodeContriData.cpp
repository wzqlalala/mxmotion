#include "mNodeContriData.h"
#include <QVector>
#include <QPair>
#include <QString>

using namespace MxFunctions;
namespace MechGlobalParamData
{
	mNodeContriData::mNodeContriData(int id, QString name)
	{
		_id = id;
		_name = name;

		//响应截断值类型("帕斯卡"/"分贝计权")
		_rpTruncType = "帕斯卡";
		//响应截断值
		_rpTruncValue = "";
		//声腔网格投影值输出
		_checkBoxCavityProj = false;;
		//峰值筛选
		_checkBoxPkScr = false;
	}

	mNodeContriData::~mNodeContriData()
	{


	}
	void mNodeContriData::setId(int id)
	{
		_id = id;
	}

	int mNodeContriData::getId()
	{
		return _id;
	}

	void mNodeContriData::setName(const QString& name)
	{
		_name = name;
	}

	QString mNodeContriData::getName()
	{
		return _name;
	}

	mNodeContriData* mNodeContriData::copyData(const int& id, const QString& name)
	{
		mNodeContriData* data = new mNodeContriData(id, name);
		data->_rpTruncType = _rpTruncType;
		data->_rpTruncValue = _rpTruncValue;
		data->_checkBoxCavityProj = _checkBoxCavityProj;
		data->_checkBoxPkScr = _checkBoxPkScr;
		return data;
	}

	MxFunctions::BrowserTreeStatus mNodeContriData::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		//if (_startID.isEmpty())
		//{
		//	info.append({ tip,"起始点编号为空" });
		//}
		return info.isEmpty() ? Tree_Yes : Tree_Failure;
	}

}
