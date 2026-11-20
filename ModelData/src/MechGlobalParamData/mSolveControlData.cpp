#include "mSolveControlData.h"

namespace MechGlobalParamData
{
	mSolveControlData::mSolveControlData()
		: _cpuCoreCount("1")
	{

	}

	mSolveControlData::~mSolveControlData()
	{


	}

	MxFunctions::BrowserTreeStatus mSolveControlData::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		bool ok = false;
		int count = _cpuCoreCount.toInt(&ok);
		if (!ok || count <= 0)
		{
			info.append({ "提示", "CPU并行核数必须为正整数" });
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
		}

		return status;
	}

	void mSolveControlData::setCpuCoreCount(const QString& value)
	{
		bool ok = false;
		int count = value.toInt(&ok);
		if (ok && count > 0)
		{
			_cpuCoreCount = QString::number(count);
		}
		else
		{
			_cpuCoreCount = "1";
		}
	}

	int mSolveControlData::getCpuCoreCount() const
	{
		bool ok = false;
		int count = _cpuCoreCount.toInt(&ok);
		if (!ok || count <= 0)
			return 1;
		return count;
	}

	QString mSolveControlData::getCpuCoreCountString() const
	{
		return _cpuCoreCount;
	}
}
