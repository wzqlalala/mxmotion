#pragma once

#include <QObject>


#include "mSectionShapeBase.h"
namespace MechSectionShapeData
{
	class Sample2D_Markers;
	class MXSECTIONSHAPEDATA_EXPORT I_SectionVisual : public MechSectionShapeData::mSectionShapeBase
	{


	public:
		I_SectionVisual(QString name="", int id=0);
		~I_SectionVisual();
		I_SectionVisual*  clone() const override;
	
		void CreateSec() override { CreateISec(); }
		void CreateISec();
		void ReBuildISec(QList<double> _para);
		friend QDataStream& operator>>(QDataStream& in, I_SectionVisual& obj);
		friend QDataStream& InStreamI_SectionVisualV3(QDataStream& in, I_SectionVisual& obj);
		friend QDataStream& operator<<(QDataStream& out, I_SectionVisual& obj);
	private:
		double a = 4, b = 6,c=6, t1 = 0.5,t2 = 0.5,t3 = 0.5;
		double _Y1 = b/2, _X1 = -a/2, _Z1 = 0;
		double _Y2 = _Y1-t2, _X2 = -a / 2, _Z2 = 0;
		double _Y3 = _Y2, _X3 = -t1/2, _Z3 = 0;
	public:
		void* _shearMarker = nullptr; //OA
		void* _a112LenDim = nullptr; //OA
		void* _E612LenDim = nullptr; //OA
		void* _E39LenDim = nullptr; //OA
		void* _P1112LenDim = nullptr; //OA
		void* _P78LenDim = nullptr; //OA
		void* _P67LenDim = nullptr; //OA
		void* _aisshape = nullptr; //OA
	};
}

