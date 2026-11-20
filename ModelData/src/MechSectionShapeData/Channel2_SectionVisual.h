#pragma once

#include <QObject>

#include "mSectionShapeBase.h"

namespace MechSectionShapeData
{
	class Sample2D_Markers;
	class MXSECTIONSHAPEDATA_EXPORT  Channel2_SectionVisual : public MechSectionShapeData::mSectionShapeBase
	{


	public:
		Channel2_SectionVisual(QString name="", int id=0);
		~Channel2_SectionVisual();
		Channel2_SectionVisual*  clone() const override;
		void CreateSec() override { CreateChan2Sec(); }
		void CreateChan2Sec();
		void ReBuildChan2Sec(QList<double> _para);
		void* _shearMarker = nullptr; //OA
		void* _a18LenDim = nullptr; //OA
		void* _a12LenDim = nullptr; //OA
		void* _a23LenDim = nullptr; //OA
		void* _aE26LenDim = nullptr; //OA

		void* _aisshape = nullptr; //OA
		friend QDataStream& operator>>(QDataStream& in, Channel2_SectionVisual& obj);
		friend QDataStream& InStreamChannel2_SectionVisualV3(QDataStream& in, Channel2_SectionVisual& obj);
		friend QDataStream& operator<<(QDataStream& out, Channel2_SectionVisual& obj);
	private:
		double a = 6, b = 6,  t1 = 1.0, t2 = 1.0;
		double _X = -b / 2.0, _Y = a/2.0,_Z = 0;

	};
}
