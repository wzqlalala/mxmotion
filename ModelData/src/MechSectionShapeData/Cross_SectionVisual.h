#pragma once

#include <QObject>

#include "mSectionShapeBase.h"
 //#include "GeoWidget.h"
namespace MechSectionShapeData
{
	class Sample2D_Markers;
	class MXSECTIONSHAPEDATA_EXPORT  Cross_SectionVisual : public MechSectionShapeData::mSectionShapeBase
	{


	public:
		Cross_SectionVisual(QString name="", int id=0);
		~Cross_SectionVisual();
		Cross_SectionVisual*  clone() const override;
		void CreateSec() override { CreateCrossSec(); }
		void CreateCrossSec();
		void ReBuildCrossSec(QList<double> _para);

		void* _shearMarker = nullptr; //OA
		void* _a1012LenDim = nullptr; //OA
		void* _E68LenDim = nullptr; //OA
		void* _a17LenDim = nullptr; //OA
		void* _a911LenDim = nullptr; //OA

		//OA Handle(AIS_Shape)  _aisshape;
		friend QDataStream& operator>>(QDataStream& in, Cross_SectionVisual& obj);
		friend QDataStream& InStreamCross_SectionVisualV3(QDataStream& in, Cross_SectionVisual& obj);
		friend QDataStream& operator<<(QDataStream& out, Cross_SectionVisual& obj);
	private:
		double DIM1 = 9, DIM2 = 1, DIM3 = 10, DIM4 = 1;

	};
}

