#pragma once

#include <QObject>
#include "mSectionShapeBase.h"

namespace MechSectionShapeData
{
	class Sample2D_Markers;
	class  MXSECTIONSHAPEDATA_EXPORT I1_SectionVisual : public MechSectionShapeData::mSectionShapeBase
	{

	public:
		I1_SectionVisual(QString name="", int id=0);
		~I1_SectionVisual();
		I1_SectionVisual*  clone() const override;
		void CreateSec() override { CreateI1Sec(); }
		void CreateI1Sec();
		void ReBuildI1Sec(QList<double> _para);
		friend QDataStream& operator>>(QDataStream& in, I1_SectionVisual& obj);
		friend QDataStream& InStreamI1_SectionVisualV3(QDataStream& in, I1_SectionVisual& obj);
		friend QDataStream& operator<<(QDataStream& out, I1_SectionVisual& obj);
	private:
		double DIM1 = 5.5, DIM2 = 0.5, DIM3 = 3.0, DIM4 = 4.0;
		double b = DIM1+DIM2, a = DIM3, t1 = (DIM4- DIM3)/2.0, t2 = DIM2;

		double _Y1 = a / 2 + t2, _X1 = -(b / 2), _Z1 = 0;
		double _Y2 = a / 2, _X2 = _X1, _Z2 = 0;
		double _Y3 = _Y2, _X3 = -(t1 / 2), _Z3 = 0;
		/*QList<double> _para;*/
	public:
		void* _shearMarker = nullptr; //OA
		void* _a811LenDim = nullptr; //OA
		void* _a23LenDim = nullptr; //OA
		void* _a16LenDim = nullptr; //OA
		void* _a310LenDim = nullptr; //OA
		
		void* _aisshape = nullptr; //OA
	};
}