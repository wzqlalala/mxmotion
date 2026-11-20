#pragma once

#include <QObject>

#include "mSectionShapeBase.h"
 //#include "GeoWidget.h"
namespace MechSectionShapeData
{
	class Sample2D_Markers;
	class MXSECTIONSHAPEDATA_EXPORT  Box1_SectionVisual : public MechSectionShapeData::mSectionShapeBase
	{
	

	public:
		Box1_SectionVisual(QString name="", int id=0);
		~Box1_SectionVisual();
		Box1_SectionVisual*  clone() const override;
		void CreateSec() override { CreateBox1Sec(); }
		void CreateBox1Sec();
		void ReBuildBox1Sec(QList<double> _para);

		void* _shearMarker = nullptr; //OA
		void* _LenDim1 = nullptr; //OA
		void* _aLenDim2 = nullptr; //OA
		void* _aLenDim3 = nullptr; //OA
		void* _LenDim4 = nullptr; //OA
		void* _LenDim5 = nullptr; //OA
		void* _LenDim6 = nullptr; //OA

		//OA Handle(AIS_Shape)  _aisshape;
		friend QDataStream& operator>>(QDataStream& in, Box1_SectionVisual& obj);
		friend QDataStream& InStreamBox1_SectionVisualV3(QDataStream& in, Box1_SectionVisual& obj);
		friend QDataStream& operator<<(QDataStream& out, Box1_SectionVisual& obj);
	private:
		double DIM1 = 10, DIM2 = 10, DIM3 = 0.5, DIM4 = 0.5,DIM5=0.5, DIM6 = 0.5;

	};
}

