#pragma once

#include "BeamSectionVisualBase.h"
#include "mSectionShapeBase.h"

namespace MechSectionShapeData
{
	class Sample2D_Markers;

	class MXSECTIONSHAPEDATA_EXPORT  Rectangle_SectionVisual : public MechSectionShapeData::mSectionShapeBase
	{


	public:
		Rectangle_SectionVisual(QString name="", int id=0);
		~Rectangle_SectionVisual();
		Rectangle_SectionVisual*  clone() const override;
		void CreateSec() override { CreatRecSec(); }
		void CreatRecSec();
		void ReBuildRecSec(QList<double> _para);

		void* _shearMarker = nullptr; //OA
		void* _a14LenDim = nullptr; //OA
		void* _a12LenDim = nullptr; //OA


		//OA Handle(AIS_Shape)  _aisshape;
		friend QDataStream& operator>>(QDataStream& in, Rectangle_SectionVisual& obj);
		friend QDataStream& InStreamRectangle_SectionVisualV3(QDataStream& in, Rectangle_SectionVisual& obj);
		friend QDataStream& operator<<(QDataStream& out, Rectangle_SectionVisual& obj);
	private:
		double a = 10, b = 10;
		double _Y=b/2, _X=-a/2,_Z=0;

		
	};
}

