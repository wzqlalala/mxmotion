#include "mPeriodicBoundary.h"
#include <QPair>
#include <QVector>
using namespace MxFunctions;
namespace MechContactData
{
	mPeriodicBoundary::mPeriodicBoundary(int id, PeriodicBoundaryType type, const QString& name, bool isUserRename)
		: _id(id), _type(type), _name(name), _isUserRename(isUserRename), _isSuppressed(false)
	{
		if (name.isEmpty())
		{
			_name = QString("%1_%2").arg(periodicBoundaryTypeToString()).arg(_id);
		}
		_pickStateMaster = 0;
		_pickStateSlave = 0;
	}

	mPeriodicBoundary::~mPeriodicBoundary()
	{
	}

	void mPeriodicBoundary::setPeriodicBoundaryId(int id)
	{
		_id = id;
	}

	int mPeriodicBoundary::getPeriodicBoundaryId() const
	{
		return _id;
	}

	bool mPeriodicBoundary::getIsUserRename() const
	{
		return _isUserRename;
	}

	void mPeriodicBoundary::setPeriodicBoundaryName(const QString& name, bool isUserRename)
	{
		_name = name;
		_isUserRename = isUserRename;
	}

	QString mPeriodicBoundary::getPeriodicBoundaryName() const
	{
		return _name;
	}

	void mPeriodicBoundary::setPeriodicBoundaryType(PeriodicBoundaryType type)
	{
		_type = type;
	}

	PeriodicBoundaryType mPeriodicBoundary::getPeriodicBoundaryType() const
	{
		return _type;
	}

	QString mPeriodicBoundary::periodicBoundaryTypeToString() const
	{
		switch (_type)
		{
		case MxFunctions::PeriodicBoundary_Translation:
			return QString("平移边界");
		case MxFunctions::PeriodicBoundary_Rotation:
			return QString("旋转边界");
		default:
			return QString("周期性边界");
		}
	}

	void mPeriodicBoundary::setDescribe(const QString& describe)
	{
		_describe = describe;
	}

	QString mPeriodicBoundary::getDescribe() const
	{
		return _describe;
	}

	mPeriodicBoundary* mPeriodicBoundary::copyPeriodicBoundary(const int& id, const QString& name)
	{
		mPeriodicBoundary* nb = new mPeriodicBoundary(id, _type, name);
		nb->_isSuppressed = _isSuppressed;
		nb->_describe = _describe;
		nb->_isUserRename = _isUserRename;
		nb->masterDir = masterDir;
		nb->slaveDir = slaveDir;
		nb->startPointId = startPointId;
		nb->startPoint = startPoint;
		nb->endPointId = endPointId;
		nb->endPoint = endPoint;
		nb->dirPickState = dirPickState;
		nb->value = value;
		nb->m_autoCalc = m_autoCalc;
		nb->m_manualValue = m_manualValue;
		nb->m_adjust = m_adjust;
		return nb;
	}

	void mPeriodicBoundary::setSuppressed(bool suppressed)
	{
		_isSuppressed = suppressed;
	}

	bool mPeriodicBoundary::isSuppressed() const
	{
		return _isSuppressed;
	}

	void mPeriodicBoundary::setPickStateMaster(int pickstate)
	{
		_pickStateMaster = pickstate;
	}

	int mPeriodicBoundary::getPickStateMaster()
	{
		return _pickStateMaster;
	}

	void mPeriodicBoundary::setPickStateSlave(int pickstate)
	{
		_pickStateSlave = pickstate;
	}

	int mPeriodicBoundary::getPickStateSlave()
	{
		return _pickStateSlave;
	}

	bool mPeriodicBoundary::isAutoCalc() const
	{
		return m_autoCalc;
	}

	void mPeriodicBoundary::setAutoCalc(bool autoCalc)
	{
		m_autoCalc = autoCalc;
	}

	double mPeriodicBoundary::getManualValue() const
	{
		return m_manualValue;
	}

	void mPeriodicBoundary::setManualValue(double value)
	{
		m_manualValue = value;
	}

	bool mPeriodicBoundary::isAdjust() const
	{
		return m_adjust;
	}

	void mPeriodicBoundary::setAdjust(bool adjust)
	{
		m_adjust = adjust;
	}

	MxFunctions::BrowserTreeStatus mPeriodicBoundary::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		if (value.isEmpty())
		{
			info.append({ "警告", getPeriodicBoundaryType() == PeriodicBoundary_Translation
				? "平移距离未输入" : "旋转角度未输入" });
		}
		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}
}
