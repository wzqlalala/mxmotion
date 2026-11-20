#include "mFrequencyPoint.h"
#include <QVector>
#include <QPair>
#include <QString>


using namespace MxFunctions;
namespace MechSetData
{
	mFrequencyPoint::mFrequencyPoint(int id, MxFunctions::SetType type, const QString& name, bool internal, bool isUserRename)
		: mSetBase(id, type, name, internal, isUserRename)
	{
	}

	mFrequencyPoint::~mFrequencyPoint()
	{
	}

	mFrequencyPoint* mFrequencyPoint::copySet(const int& id, const QString& name)
	{
		mFrequencyPoint* set = new mFrequencyPoint(id, getSetType(), name);
		set->_ids = _ids;
		set->_order = _order;
		return set;
	}

	std::vector<RenderPickResultData> mFrequencyPoint::getNodeData()
	{
		return _ids;
	}

	void mFrequencyPoint::setNodeData(std::vector<RenderPickResultData> data)
	{
		_ids = data;
	}

	bool mFrequencyPoint::getOrder()
	{
		return _isOrder;
	}

	void mFrequencyPoint::setOrder(bool order)
	{
		_isOrder = order;
	}

	int mFrequencyPoint::orderCount() const
	{
		return _order.size();
	}

	double mFrequencyPoint::orderSecondValueAt(int index) const
	{
		if (index >= 0 && index < _order.size()) {
			return _order.at(index).second;
		}
		return 0.0;
	}

	bool mFrequencyPoint::setOrderSecondValueAt(int index, double value)
	{
		if (index >= 0 && index < _order.size())
		{
			_order[index].second = value;
			return true;
		}
		return false;
	}

	void mFrequencyPoint::clearOrders()
	{
		_order.clear();
	}

	void mFrequencyPoint::addOrder(int key, double value)
	{
		_order.append(qMakePair(key, value));
	}

	bool mFrequencyPoint::insertOrder(int index, int key, double value)
	{
		if (index >= 0 && index <= _order.size())
		{
			_order.insert(index, qMakePair(key, value));
			return true;
		}
		return false;
	}

	bool mFrequencyPoint::removeOrderAt(int index)
	{
		if (index >= 0 && index < _order.size())
		{
			_order.removeAt(index);
			return true;
		}
		return false;
	}

	MxFunctions::BrowserTreeStatus mFrequencyPoint::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		if (_order.empty())
		{
			info.append({ tip,"集合数据为空" });
			return Tree_Failure;
		}
		return info.isEmpty() ? Tree_Yes : Tree_Failure;
	}
}
