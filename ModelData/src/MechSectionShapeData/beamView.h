#pragma once

//#include <Standard_Handle.hxx>
//#include <AIS_InteractiveObject.hxx>

//#include <AIS_Shape.hxx>

#include "MXSectionShapeData_global.h"
namespace MechSectionShapeData
{
	class MXSECTIONSHAPEDATA_EXPORT beamView
	{
	public:
		static QMultiHash<QString, void*> _nameAisPair; //OA
		static QMultiHash<QString, void*> _secNameAisPair; //OA
		
	};
}
