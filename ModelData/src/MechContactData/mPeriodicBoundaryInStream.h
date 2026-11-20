#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QDataStream>

#include "mPeriodicBoundaryManager.h"
#include "mPeriodicBoundary.h"
#include "mModelEnum.h"
#include "mModelStruct.h"

using namespace MxFunctions;
namespace MechContactData
{
	inline QDataStream& operator>>(QDataStream& in, mPeriodicBoundaryManager& mgr)
	{
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammPeriodicBoundaryManagerV3(in, mgr);
	    }
	    else
	    {
	        InStreammPeriodicBoundaryManagerV3(in, mgr);
	    }
	    return in;
}
	inline QDataStream& InStreammPeriodicBoundaryManagerV3(QDataStream& in, mPeriodicBoundaryManager& mgr)
	{
		in >> mgr._maxID;
		int count;
		in >> count;
		for (int i = 0; i < count; ++i)
		{
			qint32 type;
			in >> type;
			mPeriodicBoundary* obj = new mPeriodicBoundary(0, PeriodicBoundaryType(type));
			in >> *obj;

			if (obj)
				mgr.AddPeriodicBoundaryData(obj);
		}
		return in;
	}


	inline QDataStream& operator>>(QDataStream& in, mPeriodicBoundary& obj)
	{
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammPeriodicBoundaryV3(in, obj);
	    }
	    else
	    {
	        InStreammPeriodicBoundaryV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammPeriodicBoundaryV3(QDataStream& in, mPeriodicBoundary& obj)
	{
		qint32 val;
		in >> val;
		obj._type = static_cast<PeriodicBoundaryType>(val);
		in >> obj._id >> obj._name >> obj._describe >> obj._isUserRename >> obj._isSuppressed
			>> obj.masterDir >> obj.slaveDir
			>> obj.startPointId >> obj.startPoint >> obj.endPointId >> obj.endPoint
			>> obj.value
			>> obj.dirPickState
			>> obj._pickStateMaster
			>> obj._pickStateSlave
			>> obj.m_autoCalc
			>> obj.m_manualValue
			>> obj.m_adjust;
		return in;
	}

}
