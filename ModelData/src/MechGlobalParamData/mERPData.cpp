#include "mERPData.h"
#include <QVector>
#include <QPair>
#include <QString>

using namespace MxFunctions;
namespace MechGlobalParamData
{
	mERPData::mERPData(int id, QString name)
	{
		_id = id;
		_name = name;

		//介质密度(实数>0.0),默认=1.0
		_erprho = "1.0";
		//声速(实数>0.0),默认=1.0
		_erpc = "1.0";
		//缩放系数(实数>0.0),默认=1.0
		_rhocp = "1.0";
		//辐射损耗密度(实数>0.0),默认=1.0
		_erprlf = "1.0";
		//参考值(实数>0.0),默认=1.0
		_erprefdb = "1.0";
	}

	mERPData::~mERPData()
	{


	}
	void mERPData::setId(int id)
	{
		_id = id;
	}

	int mERPData::getId()
	{
		return _id;
	}

	void mERPData::setName(const QString& name)
	{
		_name = name;
	}

	QString mERPData::getName()
	{
		return _name;
	}

	mERPData* mERPData::copyData(const int& id, const QString& name)
	{
		mERPData* data = new mERPData(id, name);
		data->_erprho = _erprho;
		data->_erpc = _erpc;
		data->_rhocp = _rhocp;
		data->_erprlf = _erprlf;
		data->_erprefdb = _erprefdb;
		return data;
	}

	MxFunctions::BrowserTreeStatus mERPData::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		return info.isEmpty() ? Tree_Yes : Tree_Failure;
	}

}
