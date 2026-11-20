#pragma once

#include <QObject>

#include "mSectionShapeBase.h"
// #include "GeoWidget.h"
namespace MechSectionShapeData
{
	class Sample2D_Markers;
	class MXSECTIONSHAPEDATA_EXPORT  T_SectionVisual : public MechSectionShapeData::mSectionShapeBase
	{

	public:
		T_SectionVisual(QString name="", int id=0);
		~T_SectionVisual();
		T_SectionVisual* clone() const override;
		void CreateSec() override { CreateTSec(); }
		void CreateTSec();
		void ReBuildTSec(QList<double> _para);

		void* _shearMarker = nullptr; //OA
		void* _a18LenDim = nullptr; //OA
		void* _a12LenDim = nullptr; //OA
		void* _a36LenDim = nullptr; //OA
		void* _E84LenDim = nullptr; //OA
	
		//OA Handle(AIS_Shape)  _aisshape;
		friend QDataStream& operator>>(QDataStream& in, T_SectionVisual& obj);
		friend QDataStream& InStreamT_SectionVisualV3(QDataStream& in, T_SectionVisual& obj);
		friend QDataStream& operator<<(QDataStream& out, T_SectionVisual& obj);
	private:
		double a = 6, b = 6, c = 6, t1 = 0.5, t2 = 0.5;
		double _Y1 = b/2, _X1 = -a/2, _Z1 = 0;
		double _Y2 = _Y1-t2, _X2 = _X1, _Z2 = 0;
		double _Y3 = _Y2, _X3 = -t1/2, _Z3 = 0;


	};
}

