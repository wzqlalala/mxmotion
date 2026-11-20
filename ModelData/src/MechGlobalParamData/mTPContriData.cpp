#include "mTPContriData.h"
#include <QVector>
#include <QPair>
#include <QString>

using namespace MxFunctions;
namespace MechGlobalParamData
{
	mTPContriData::mTPContriData(int id, QString name)
	{
		_id = id;
		_name = name;

		//响应类型下拉框 disp、vel、acc
		_respType = "disp";
	}

	mTPContriData::~mTPContriData()
	{


	}
	void mTPContriData::setId(int id)
	{
		_id = id;
	}

	int mTPContriData::getId()
	{
		return _id;
	}

	void mTPContriData::setName(const QString& name)
	{
		_name = name;
	}

	QString mTPContriData::getName()
	{
		return _name;
	}

	mTPContriData* mTPContriData::copyData(const int& id, const QString& name)
	{
		mTPContriData* data = new mTPContriData(id, name);
		data->_respType = _respType;
		return data;
	}

	MxFunctions::BrowserTreeStatus mTPContriData::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		//if (_startID.isEmpty())
		//{
		//	info.append({ tip,"起始点编号为空" });
		//}
		return info.isEmpty() ? Tree_Yes : Tree_Failure;
	}

}
