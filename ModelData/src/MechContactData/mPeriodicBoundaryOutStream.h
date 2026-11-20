#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QDataStream>
#include <vector>

#include "mPeriodicBoundary.h"
#include "mPeriodicBoundaryManager.h"
#include "mModelEnum.h"
#include "mModelStruct.h"

namespace MechContactData
{
	inline QDataStream& operator<<(QDataStream& out, mPeriodicBoundaryManager& mgr)
	{
		out << mgr._maxID;
		std::vector<int> ids = mgr.getAllPeriodicBoundaryID();
		int count = ids.size();
		out << count;
		for (int i = 0; i < count; ++i)
		{
			mPeriodicBoundary* obj = mgr.getPeriodicBoundaryData(ids[i]);
			MxFunctions::PeriodicBoundaryType type = obj->getPeriodicBoundaryType();
			out << (qint32)type;
			out << *obj;
		}
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, mPeriodicBoundary& obj)
	{
		qint32 val = obj._type;
		out << val;
		out << obj._id << obj._name << obj._describe << obj._isUserRename << obj._isSuppressed
			<< obj.masterDir << obj.slaveDir
			<< obj.startPointId << obj.startPoint << obj.endPointId << obj.endPoint
			<< obj.value
			<< obj.dirPickState
			<< obj._pickStateMaster
			<< obj._pickStateSlave
			<< obj.m_autoCalc
			<< obj.m_manualValue
			<< obj.m_adjust;
		return out;
	}
}
