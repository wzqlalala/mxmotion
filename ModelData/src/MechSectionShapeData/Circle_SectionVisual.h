#pragma once

#include <QObject>
//#include "AIS_RadiusDimension.hxx"
#include "mSectionShapeBase.h"
namespace MechSectionShapeData
{
	class Sample2D_Markers;
	class  MXSECTIONSHAPEDATA_EXPORT Circle_SectionVisual : public MechSectionShapeData::mSectionShapeBase
	{


	public:
		Circle_SectionVisual(QString name="", int id=0);
		~Circle_SectionVisual();
		Circle_SectionVisual*  clone() const override;
		void CreateSec() override { CreateCirSec(); }
		void CreateCirSec();
		void ReBuildCirSec(QList<double> _para);
		double _R = 10;
		
		void* _shearMarker = nullptr; //OA
		void* _a14LenDim = nullptr; //OA
	

		//OA Handle(AIS_Shape)  _aisshape;
		friend QDataStream& operator>>(QDataStream& in, Circle_SectionVisual& obj);
		friend QDataStream& InStreamCircle_SectionVisualV3(QDataStream& in, Circle_SectionVisual& obj);
		friend QDataStream& operator<<(QDataStream& out, Circle_SectionVisual& obj);
	};
}

