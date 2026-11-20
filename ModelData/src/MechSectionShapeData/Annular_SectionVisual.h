#pragma once

#include <QObject>

#include "mSectionShapeBase.h"
//OA #include "AIS_RadiusDimension.hxx"
namespace MechSectionShapeData
{
	class Sample2D_Markers;
	class MXSECTIONSHAPEDATA_EXPORT  Annular_SectionVisual : public MechSectionShapeData::mSectionShapeBase
	{
	
		

	public:
		Annular_SectionVisual(QString name="", int id = 0);
		~Annular_SectionVisual();
		Annular_SectionVisual* clone() const override;
		void CreateSec() override { CreateAnuSec(); }
		void CreateAnuSec();
		void ReBuildAnnSec(QList<double> _para);

		void* _shearMarker = nullptr; //OA
		void* _a14LenDim = nullptr; //OA
		void* _aLenDim = nullptr; //OA
	
		//OA Handle(AIS_Shape)  _aisshape;
		friend QDataStream& operator>>(QDataStream& in, Annular_SectionVisual& obj);
		friend QDataStream& InStreamAnnular_SectionVisualV3(QDataStream& in, Annular_SectionVisual& obj);
		friend QDataStream& operator<<(QDataStream& out, Annular_SectionVisual& obj);
	private:
		double _R1 = 6;
		double _R2 = 5;


	
	};
}

