#pragma once

#include <QObject>

#include "mSectionShapeBase.h"
 //#include "GeoWidget.h"
namespace MechSectionShapeData
{
	class Sample2D_Markers;
	class MXSECTIONSHAPEDATA_EXPORT  Z_SectionVisual : public   MechSectionShapeData::mSectionShapeBase
	{


	public:
		Z_SectionVisual(QString name="", int id=0);
		~Z_SectionVisual();
		Z_SectionVisual*  clone() const override;
		void CreateSec() override { CreateZSec(); }
		void CreateZSec();
		void ReBuildZSec(QList<double> _para);
		void* _shearMarker = nullptr; //OA
		void* _a12LenDim = nullptr; //OA
		void* _E27LenDim = nullptr; //OA
		void* _a3LenDim = nullptr; //OA
		void* _a15LenDim = nullptr; //OA

		//OA Handle(AIS_Shape)  _aisshape;
		friend QDataStream& operator>>(QDataStream& in, Z_SectionVisual& obj);
		friend QDataStream& InStreamZ_SectionVisualV3(QDataStream& in, Z_SectionVisual& obj);
		friend QDataStream& operator<<(QDataStream& out, Z_SectionVisual& obj);
	private:
		double DIM1 = 5.5, DIM2 = 0.5, DIM3 = 7, DIM4 = 8;
		
	
	};
}

