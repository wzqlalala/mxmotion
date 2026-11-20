#pragma once

#include <QObject>

#include "mSectionShapeBase.h"

namespace MechSectionShapeData
{
	class Sample2D_Markers;
	class MXSECTIONSHAPEDATA_EXPORT  Channel1_SectionVisual : public MechSectionShapeData::mSectionShapeBase
	{


	public:
		Channel1_SectionVisual(QString name="", int id=0);
		~Channel1_SectionVisual();
		Channel1_SectionVisual*  clone() const override;
		void CreateSec() override { CreateChan1Sec(); }
		void CreateChan1Sec();
		void ReBuildChan1Sec(QList<double> _para);
		//OA Handle(Sample2D_Markers) _shearMarker;
		//OA Handle(AIS_LengthDimension) _aR18LenDim; //DIM1
		//OA Handle(AIS_LengthDimension) _aL18LenDim; //DIM2
		//OA Handle(AIS_LengthDimension) _a16LenDim; //DIM4
		//OA Handle(AIS_LengthDimension) _a25LenDim; //DIM3

		//OA Handle(AIS_Shape)  _aisshape;
		friend QDataStream& operator>>(QDataStream& in, Channel1_SectionVisual& obj);
		friend QDataStream& InStreamChannel1_SectionVisualV3(QDataStream& in, Channel1_SectionVisual& obj);
		friend QDataStream& operator<<(QDataStream& out, Channel1_SectionVisual& obj);

	private:
		double DIM1 = 3.0, DIM2 = 3.0, DIM3 = 4.0, DIM4 = 6.0;
		double a = DIM4, b = DIM3, t1 = DIM1, t2 = DIM2;
		double _X = t1, _Y = a/2.0,_Z = 0;

	};
}

