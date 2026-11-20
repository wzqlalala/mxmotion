#include "mGenericParamData.h"
#include <QVector>
#include <QPair>
#include <QString>

using namespace MxFunctions;
namespace MechGlobalParamData
{
	mGenericParamData::mGenericParamData(int id, QString name)
	{
		_id = id;
		_name = name;

		_method = "GUYAN";
		_upFrequency = "";
		_order = "";
		_startID = "";
	}

	mGenericParamData::~mGenericParamData()
	{


	}
	void mGenericParamData::setId(int id)
	{
		_id = id;
	}

	int mGenericParamData::getId()
	{
		return _id;
	}

	void mGenericParamData::setName(const QString& name)
	{
		_name = name;
	}

	QString mGenericParamData::getName()
	{
		return _name;
	}

	mGenericParamData* mGenericParamData::copyData(const int& id, const QString& name)
	{
		mGenericParamData* data = new mGenericParamData(id, name);
		data->_method = _method;
		data->_upFrequency = _upFrequency;
		data->_order = _order;
		data->_startID = _startID;

		return data;
	}

	MxFunctions::BrowserTreeStatus mGenericParamData::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		if (_method != "GUYAN" && _startID.isEmpty())
		{
			info.append({ tip,"起始点编号为空" });
		}
		return info.isEmpty() ? Tree_Yes : Tree_Failure;
	}

}
