#pragma once

#include <QObject>

#include "mSectionShapeBase.h"
 //#include "GeoWidget.h"
namespace MechSectionShapeData
{
	class Sample2D_Markers;
	class MXSECTIONSHAPEDATA_EXPORT  Hat_SectionVisual : public  MechSectionShapeData::mSectionShapeBase
	{


	public:
		Hat_SectionVisual(QString name="", int id=0);
		~Hat_SectionVisual();
		Hat_SectionVisual*  clone() const override;
		void CreateSec() override { CreateHatSec(); }
		void CreateHatSec();
		void ReBuildHatSec(QList<double> _para);

		void* _shearMarker = nullptr; //OA
		void* _a19LenDim = nullptr; //OA
		void* _a812LenDim = nullptr; //OA
		void* _a112LenDim = nullptr; //OA
		void* _P23LenDim = nullptr; //OA

		//OA Handle(AIS_Shape)  _aisshape;
		friend QDataStream& operator>>(QDataStream& in, Hat_SectionVisual& obj);
		friend QDataStream& InStreamHat_SectionVisualV3(QDataStream& in, Hat_SectionVisual& obj);
		friend QDataStream& operator<<(QDataStream& out, Hat_SectionVisual& obj);
	private:
		double DIM1 = 6, DIM2 = 0.5, DIM3 = 4, DIM4 = 1;

	};
}

