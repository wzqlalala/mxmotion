#include "mFreqSamplPointBase.h"

using namespace MxFunctions;
namespace MechCaseData
{
	mFreqSamplPointBase::mFreqSamplPointBase(int id, const QString& name)
		:_id(id), _freqSamplPointName(name)
	{
		if (name.isEmpty())
		{
			_freqSamplPointName = QString("Freq_%1").arg(_id);
		}

		_checked = { false,false ,false ,false ,false };
		_freqSamplPoint = {};
		_freqSamplPoint1 = {};
		_freqSamplPoint2 = {};
		_freqSamplPoint3 = {};
		_freqSamplPoint4 = {};
	}

	mFreqSamplPointBase::~mFreqSamplPointBase()
	{
	}

	void mFreqSamplPointBase::setFreqSamplPointId(int id)
	{
		_id = id;
	}


	int mFreqSamplPointBase::getFreqSamplPointId()
	{
		return _id;
	}

	void mFreqSamplPointBase::setFreqSamplPointName(const QString& name)
	{
		_freqSamplPointName = name;
	}

	QString mFreqSamplPointBase::getFreqSamplPointName()
	{
		return _freqSamplPointName;
	}

	mFreqSamplPointBase* mFreqSamplPointBase::copyFreqSamplPoint(const int& id, const QString& name)
	{
		mFreqSamplPointBase* freq = new mFreqSamplPointBase(id, name);

		freq->_freqSamplPoint = _freqSamplPoint;
		freq->_freqSamplPoint1 = _freqSamplPoint1;
		freq->_freqSamplPoint2 = _freqSamplPoint2;
		freq->_freqSamplPoint3 = _freqSamplPoint3;
		freq->_freqSamplPoint4 = _freqSamplPoint4;

		return freq;
	}

	MxFunctions::BrowserTreeStatus mFreqSamplPointBase::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		if (!_checked.contains(true))
		{
			info.append({ tip,"请至少选择一种FREQ采样频率点方式" });
		}
		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;

	}
}