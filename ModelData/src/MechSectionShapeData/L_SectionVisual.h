#pragma once

#include <QObject>

//#include "TopoDS_Shape.hxx"
#include "BeamSectionVisualBase.h"
#include "MXSectionShapeData_global.h"
#include "mSectionShapeBase.h"
//OA #include <AIS_LengthDimension.hxx>
namespace MechSectionShapeData
{
	class Sample2D_Markers;
	class  MXSECTIONSHAPEDATA_EXPORT L_SectionVisual : public MechSectionShapeData::mSectionShapeBase
	{
		/*Q_OBJECT*/

	public:
		L_SectionVisual(QString name="", int id=0);
		~L_SectionVisual();
		L_SectionVisual*  clone() const override;
		void CreateSec() override { CreateLSec(); }
		void CreateLSec();
		void ReBuildLSec(QList<double> _para);
		void initData();
		friend QDataStream& operator>>(QDataStream& in, L_SectionVisual& obj);
		friend QDataStream& InStreamL_SectionVisualV3(QDataStream& in, L_SectionVisual& obj);
		friend QDataStream& operator<<(QDataStream& out, L_SectionVisual& obj);
	private:
		double DIM1 = 6, DIM2 = 6, DIM3 = 0.5, DIM4 = 1;

		double a = 6;
		double b = 6;
		double t1 = 0.5;
		double t2 = 1;
		double _Y = a/2;
		double _X = -b/2;
		double _Z = 0;
		
	
		QString _name;
		//MxBeam::GeoWidget*_geoWindow;
	public:
		void* _shearMarker = nullptr; //OA
		void* _a16LenDim = nullptr; //OA
		void* _a12LenDim = nullptr; //OA
		void* _a23LenDim = nullptr; //OA
		void* _a34LenDim = nullptr; //OA
		void* _aisshape = nullptr; //OA
	};
}

