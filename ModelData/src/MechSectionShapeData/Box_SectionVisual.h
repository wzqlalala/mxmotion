#pragma once

#include <QObject>
#include "mSectionShapeBase.h"

namespace MechSectionShapeData
{
	class Sample2D_Markers;
	class MXSECTIONSHAPEDATA_EXPORT Box_SectionVisual : public MechSectionShapeData::mSectionShapeBase
	{
		

	public:
		Box_SectionVisual(QString name="", int id=0);
		~Box_SectionVisual();
		Box_SectionVisual*  clone() const override;
		void CreateSec() override { CreateBoxSec(); }
		void CreateBoxSec();
		void ReBuildBoxSec(QList<double> _para);

		void* _shearMarker = nullptr; //OA
		void* _a14LenDim = nullptr; //OA
		void* _a12LenDim = nullptr; //OA
		void* _E37LenDim = nullptr; //OA
		void* _E26LenDim = nullptr; //OA

		//OA Handle(AIS_Shape)  _aisshape;
		friend QDataStream& operator>>(QDataStream& in, Box_SectionVisual& obj);
		friend QDataStream& InStreamBox_SectionVisualV3(QDataStream& in, Box_SectionVisual& obj);
		friend QDataStream& operator<<(QDataStream& out, Box_SectionVisual& obj);
	private:
		double DIM1 = 10, DIM2 = 10, DIM3 = 0.5, DIM4 = 0.5;
		double a = DIM1, b = DIM2, t1 = DIM3, t2 = DIM4,_X = -b / 2, _Y = a / 2,_Z = 0;


	};
}

