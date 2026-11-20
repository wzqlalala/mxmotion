#pragma once

#include <QObject>

//#include "BeamSectionVisualBase.h"
//#include "TopoDS_Shape.hxx"
#include <qhash.h>
//#include <AIS_LengthDimension.hxx>
#include "mguibeam_global.h"
#include "MXSectionShapeData_global.h"
#include "mSectionShapeBase.h"
namespace MechSectionShapeData
{
	class Sample2D_Markers;

	class MXSECTIONSHAPEDATA_EXPORT  H_SectionVisual : public MechSectionShapeData::mSectionShapeBase
	{
	/*	Q_OBJECT*/

	public:
		H_SectionVisual(QString name="", int id=0);
		~H_SectionVisual();
		H_SectionVisual* clone() const override;
		void CreateSec() override { CreateHSec(); }
		void CreateHSec();
		void ReBuildHSec(QList<double> _para);
		friend QDataStream& operator>>(QDataStream& in, H_SectionVisual& obj);
		friend QDataStream& InStreamH_SectionVisualV3(QDataStream& in, H_SectionVisual& obj);
		friend QDataStream& operator<<(QDataStream& out, H_SectionVisual& obj);
	private:
		double DIM1 = 3, DIM2 = 1, DIM3 = 6, DIM4 = 0.5;

		double a = DIM1,b= DIM3,t2=0.5*DIM2,t1= DIM4;
		double _X1 = -(a/2+t2), _Y1 = (b/2.0), _Z1 = 0;
		double _X2 =- a / 2, _Y2 = (b / 2.0), _Z2 = 0;
		double _X3 = _X2, _Y3 =t1/2, _Z3 = 0;
		//MxBeam::GeoWidget *_geoWindow;
	public:
		void* _shearMarker = nullptr; //OA
		void* _a25LenDim = nullptr; //OA
		void* _a67LenDim = nullptr; //OA
		void* _a1112LenDim = nullptr; //OA
		void* _a310LenDim = nullptr; //OA
		void* _aisshape = nullptr; //OA


	};
}

