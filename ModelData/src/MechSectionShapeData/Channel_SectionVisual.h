#pragma once

#include <QObject>
#include "mSectionShapeBase.h"

namespace MechSectionShapeData
{
	class Sample2D_Markers;
	class MXSECTIONSHAPEDATA_EXPORT  Channel_SectionVisual : public MechSectionShapeData::mSectionShapeBase
	{
		

	public:
		Channel_SectionVisual(QString name="", int id=0);
		~Channel_SectionVisual();
		Channel_SectionVisual*  clone() const override;
		void CreateSec() override { CreateChanSec(); }
		void CreateChanSec();
		void ReBuildChanSec(QList<double> _para);
		void* _shearMarker = nullptr; //OA
		void* _a14LenDim = nullptr; //OA
		void* _a12LenDim = nullptr; //OA
		void* _a84LenDim = nullptr; //OA
		void* _aE26LenDim = nullptr; //OA

		//OA Handle(AIS_Shape)  _aisshape;
		friend QDataStream& operator>>(QDataStream& in, Channel_SectionVisual& obj);
		friend QDataStream& InStreamChannel_SectionVisualV3(QDataStream& in, Channel_SectionVisual& obj);
		friend QDataStream& operator<<(QDataStream& out, Channel_SectionVisual& obj);
	private:
		double DIM3 = 0.5, DIM4 = 1.0, DIM1 = 6.0, DIM2 = 6.0;
		double _X = -DIM1 / 2.0, _Y = DIM2 / 2.0, _Z = 0;

	};

}
