#include "mPanelContriData.h"
#include <QVector>
#include <QPair>
#include <QString>

using namespace MxFunctions;
namespace MechGlobalParamData
{
	mPanelContriData::mPanelContriData(int id, QString name)
	{
		_id = id;
		_name = name;

		//输出类型下拉框 幅值、实部、虚部
		_outPutType = "实部";
		//峰值筛选
		_checkBoxPkScr = false;
	}

	mPanelContriData::~mPanelContriData()
	{


	}
	void mPanelContriData::setId(int id)
	{
		_id = id;
	}

	int mPanelContriData::getId()
	{
		return _id;
	}

	void mPanelContriData::setName(const QString& name)
	{
		_name = name;
	}

	QString mPanelContriData::getName()
	{
		return _name;
	}

	mPanelContriData* mPanelContriData::copyData(const int& id, const QString& name)
	{
		mPanelContriData* data = new mPanelContriData(id, name);
		data->_outPutType = _outPutType;
		data->_checkBoxPkScr = _checkBoxPkScr;
		return data;
	}

	MxFunctions::BrowserTreeStatus mPanelContriData::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		//if (_startID.isEmpty())
		//{
		//	info.append({ tip,"起始点编号为空" });
		//}
		return info.isEmpty() ? Tree_Yes : Tree_Failure;
	}

}
