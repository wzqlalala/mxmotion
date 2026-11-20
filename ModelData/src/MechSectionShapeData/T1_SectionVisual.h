#pragma once

#include <QObject>

//#include "BeamSectionVisualBase.h"
 //#include "GeoWidget.h"
#include "mSectionShapeBase.h"
namespace MechSectionShapeData
{
	class Sample2D_Markers;
	class MXSECTIONSHAPEDATA_EXPORT  T1_SectionVisual :   public MechSectionShapeData::mSectionShapeBase
	{


	public:
		T1_SectionVisual(QString name="", int id=0);
		~T1_SectionVisual();
		T1_SectionVisual*  clone() const override;
		void CreateSec() override { CreateT1Sec(); }
		void CreateT1Sec();
		void ReBuildT1Sec(QList<double> _para);


		void* _shearMarker = nullptr; //OA
		void* _a45LenDim = nullptr; //OA
		void* _E68LenDim = nullptr; //OA
		void* _a34LenDim = nullptr; //OA
		void* _a27LenDim = nullptr; //OA

		//OA Handle(AIS_Shape)  _aisshape;
		friend QDataStream& operator>>(QDataStream& in, T1_SectionVisual& obj);
		friend QDataStream& InStreamT1_SectionVisualV3(QDataStream& in, T1_SectionVisual& obj);
		friend QDataStream& operator<<(QDataStream& out, T1_SectionVisual& obj);
	private:
		double DIM1 = 6, DIM2 = 6, DIM3 = 0.5, DIM4 = 0.5;
		double _Y1 = DIM4 /2, _X1 = -DIM2 /2, _Z1 = 0;
		double _Y2 = DIM4 / 2, _X2 = DIM2 / 2, _Z2 = 0;
		double _Y3 = DIM1 / 2, _X3 = DIM2 / 2, _Z3 = 0;
		double _Y4 = DIM1 / 2, _X4 = DIM2 / 2+DIM3, _Z4 = 0;


	};
}

