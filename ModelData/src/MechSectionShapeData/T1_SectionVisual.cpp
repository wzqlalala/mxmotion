#include "T1_SectionVisual.h"
//#include "gp_Pnt.hxx"
//#include "BRepBuilderAPI_MakeEdge.hxx"
//#include "BRepBuilderAPI_MakeFace.hxx"
//#include "BRepBuilderAPI_MakeWire.hxx"
//#include "TopoDS_Face.hxx"
//#include "TopoDS_Wire.hxx"

//#include <Prs3d_Drawer.hxx>
//#include <AIS_InteractiveContext.hxx>
//#include <AIS_LengthDimension.hxx>
//#include "GeoWidget.h"
//#include "GC_MakePlane.hxx"

#include "mPBeam.h"
#include "Sample2D_Markers.h"
//#include <BRepGProp.hxx>
//#include <GProp_GProps.hxx>
#include <mGlobalSignals.h>

#include "beamView.h"

namespace MechSectionShapeData
{
	T1_SectionVisual::T1_SectionVisual(QString name, int id)
		: mSectionShapeBase(Sec_T1,name,id)
	{
		_para.push_back(DIM1);
		_para.push_back(DIM2);
		_para.push_back(DIM3);
		_para.push_back(DIM4);
	}

	T1_SectionVisual::~T1_SectionVisual()
	{
	}
	T1_SectionVisual* T1_SectionVisual::clone() const
	{
		T1_SectionVisual* newSec = new T1_SectionVisual(_sectionShapeName, _id);

		newSec->DIM1 = DIM1;
		newSec->DIM2 = DIM2;
		newSec->DIM3 = DIM3;
		newSec->DIM4 = DIM4;
		newSec->_Y1 = _Y1;
		newSec->_Z1 = _Z1;
		newSec->_X2 = _X2;
		newSec->_Y2 = _Y2;
		newSec->_Z2 = _Z2;
		newSec->_X3 = _X3;
		newSec->_Y3 = _Y3;
		newSec->_Z3 = _Z3;
		newSec->_X4 = _X4;
		newSec->_Y4 = _Y4;
		newSec->_Z4 = _Z4;


		newSec->_shearMarker = _shearMarker;
		newSec->_a45LenDim = _a45LenDim;
		newSec->_E68LenDim = _E68LenDim;
		newSec->_a34LenDim = _a34LenDim;
		newSec->_a27LenDim = _a27LenDim;
		//OA newSec->_aisshape = _aisshape;
		newSec->_para = _para;
		newSec->_data = _data;
		newSec->area = area;
		newSec->Iz = Iz;
		newSec->Iy = Iy;
		newSec->J = J;
		newSec->Iyz = Iyz;
		newSec->K1 = K1;
		newSec->K2 = K2;

		return newSec;
	}
	void T1_SectionVisual::CreateT1Sec()
	{
		double area, Iz, Iy, J, Iyz, K1, K2;
		try
		{

			//OA gp_Pnt Point1(_X1, _Y1, _Z1); gp_Pnt Point8(_X1, -_Y1, _Z1);
			//OA gp_Pnt Point2(_X2, _Y2, _Z2); gp_Pnt Point7(_X2, -_Y2, _Z2);
			//OA gp_Pnt Point3(_X3, _Y3, _Z3); gp_Pnt Point6(_X3, -_Y3, _Z3);
			//OA gp_Pnt Point4(_X4, _Y4, _Z4); gp_Pnt Point5(_X4, -_Y4, _Z4);
			

			//OA gp_Pnt Point27(_X2, 0 , _Z3); gp_Pnt Point34((_X3 + _X4)/2.0, _Y4+0.5, _Z3); gp_Pnt Point45(_X4+0.5,0, _Z3); gp_Pnt PointE68(0, -_Y3-0.5, _Z3);
			//OA gp_Pnt shearCenter(DIM2/2, 0, _Z3);
			//OA TopoDS_Edge Edge1 = BRepBuilderAPI_MakeEdge(Point1, Point2);
			//OA TopoDS_Edge Edge2 = BRepBuilderAPI_MakeEdge(Point2, Point3);
			//OA TopoDS_Edge Edge3 = BRepBuilderAPI_MakeEdge(Point3, Point4);
			//OA TopoDS_Edge Edge4 = BRepBuilderAPI_MakeEdge(Point4, Point5);
			//OA TopoDS_Edge Edge5 = BRepBuilderAPI_MakeEdge(Point5, Point6);
			//OA TopoDS_Edge Edge6 = BRepBuilderAPI_MakeEdge(Point6, Point7);
			//OA TopoDS_Edge Edge7 = BRepBuilderAPI_MakeEdge(Point7, Point8);
			//OA TopoDS_Edge Edge8 = BRepBuilderAPI_MakeEdge(Point8, Point1);
			//OA auto TSecWire1 = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//OA TopoDS_Wire TSecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6, Edge7, Edge8);
			//OA TSecWire1.Add(TSecWire2);
			//OA TopoDS_Shape TSecFace = BRepBuilderAPI_MakeFace(TSecWire1);
			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(TSecFace);
			//OA _aisshape = aisshape;
			//OA BaseshowBeamSec(aisshape);
			
			QString name = mPBeam::getInstance()->setDefaultNameForBeam(Sec_T1);

			//emit AddSecSignal(name);
			//emit showParaOnPanel(_para, name);
			//OA beamView::_nameAisPair.insert(name, aisshape);
			//OA beamView::_secNameAisPair.insert(name, aisshape);
			mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());
			//OA getDataFromBeam(name, TSecFace);
			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(shearCenter.X(), shearCenter.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//_geoWindow->getAisContext()->Display(shearMarker, Standard_False);
			//OA beamView::_nameAisPair.insert(name, shearMarker);
			//OA _shearMarker = shearMarker;
			//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();


			//OA Handle(AIS_LengthDimension) a45LenDim = new AIS_LengthDimension(Edge4, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an45Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString aValue = a45LenDim->GetValue();
			//OA TCollection_AsciiString atext = "DIM1 = ";
			//OA an45Aspect->SetValueStringFormat(atext + aValue);
			//OA an45Aspect->MakeArrows3d(Standard_False);
			//OA an45Aspect->MakeText3d(Standard_False);
			//OA an45Aspect->TextAspect()->SetHeight(0.3);
			//OA an45Aspect->ArrowAspect()->SetLength(0.2);
			//OA an45Aspect->MakeTextShaded(Standard_True);
			//OA an45Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an45Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an45Aspect->SetArrowTailSize(0.3);
			//OA an45Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a45LenDim->SetTextPosition(Point45);
			//OA a45LenDim->SetDimensionAspect(an45Aspect);
			//_geoWindow->getAisContext()->Display(a45LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a45LenDim);
			//OA _a45LenDim = a45LenDim;

			//OA Handle(AIS_LengthDimension) E68LenDim = new AIS_LengthDimension(Edge6, Edge8, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an68Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValue = E68LenDim->GetValue();
			//OA TCollection_AsciiString btext = "DIM2 = ";
			//OA an68Aspect->SetValueStringFormat(btext + bValue);
			//OA an68Aspect->MakeArrows3d(Standard_False);
			//OA an68Aspect->MakeText3d(Standard_False);
			//OA an68Aspect->TextAspect()->SetHeight(0.3);
			//OA an68Aspect->ArrowAspect()->SetLength(0.2);
			//OA an68Aspect->MakeTextShaded(Standard_True);
			//OA an68Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an68Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an68Aspect->SetArrowTailSize(0.3);
			//OA an68Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA E68LenDim->SetTextPosition(PointE68);
			//OA E68LenDim->SetDimensionAspect(an68Aspect);
			//_geoWindow->getAisContext()->Display(E68LenDim, true);
			//OA beamView::_nameAisPair.insert(name, E68LenDim);
			//OA _E68LenDim = E68LenDim;


			//OA Handle(AIS_LengthDimension) a34LenDim = new AIS_LengthDimension(Edge3, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an34Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t2Value = a34LenDim->GetValue();
			//OA TCollection_AsciiString t2text = "DIM3 = ";
			//OA an34Aspect->SetValueStringFormat(t2text + t2Value);
			//OA an34Aspect->MakeArrows3d(Standard_False);
			//OA an34Aspect->MakeText3d(Standard_False);
			//OA an34Aspect->TextAspect()->SetHeight(0.3);
			//OA an34Aspect->ArrowAspect()->SetLength(0.2);
			//OA an34Aspect->MakeTextShaded(Standard_True);
			//OA an34Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an34Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an34Aspect->SetArrowTailSize(0.3);
			//OA an34Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a34LenDim->SetTextPosition(Point34);
			//OA a34LenDim->SetDimensionAspect(an34Aspect);
			//_geoWindow->getAisContext()->Display(a34LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a34LenDim);
			//OA _a34LenDim = a34LenDim;
			//OA Handle(AIS_LengthDimension) a27LenDim = new AIS_LengthDimension(Point2, Point7, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an27Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t1Value = a27LenDim->GetValue();
			//OA TCollection_AsciiString t1text = "DIM4 = ";
			//OA an27Aspect->SetValueStringFormat(t1text + t1Value);
			//OA an27Aspect->MakeArrows3d(Standard_False);
			//OA an27Aspect->MakeText3d(Standard_False);
			//OA an27Aspect->TextAspect()->SetHeight(0.3);
			//OA an27Aspect->ArrowAspect()->SetLength(0.2);
			//OA an27Aspect->MakeTextShaded(Standard_True);
			//OA an27Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an27Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an27Aspect->SetArrowTailSize(0.3);
			//OA an27Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a27LenDim->SetTextPosition(Point27);
			//OA a27LenDim->SetDimensionAspect(an27Aspect);
			//_geoWindow->getAisContext()->Display(a27LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a27LenDim);
			//OA _a27LenDim = a27LenDim;
			

			double a = DIM1;
			double b = DIM2 + DIM3;
			double t1 = DIM4;
			double t2 = DIM3;
			 area = a * t2 + (b - t2) * t1;
			double Y = ((a - t1) * t2 * (b / 2.0 - t2 / 2.0) + 0) / area;
			double Iz = (t2 * pow(a, 3) + (b - t2) * pow(t1, 3)) / 12.0;
			double S1 = (a - t1) * t2, Y1 = b / 2.0 - t2 / 2.0,
				I1Z = (a - t1) * pow(t2, 3) / 12.0 + S1 * (Y - Y1) * (Y - Y1),
				S2 = b * t1, Z2 = 0, Y2 = 0,
				I2Z = t1 * pow(b, 3) / 12.0 + S2 * Y * Y;
			 Iy = I1Z + I2Z;
			 Iyz = 0;
			// double beta1 = gettorsioncoefficient(a, t2);
			// double beta2 = gettorsioncoefficient(b - t2, t1);
			// double beta3 = gettorsioncoefficient(t2, t1);
			// double It1 = beta1 * a * pow(t2, 3.0),
			//        It2 = beta2 * (b - t2) * pow(t1, 3.0);
			double HV = a; double TV = t2; double WH = b - t2; double TH = t1;
			double J = 1.0 / 3.0 * (WH * TH * TH * TH + HV * TV * TV * TV);
			double JCONS = HV * TV / ((WH + 0.5 * TV) * TH);
			double KCONS = HV / (WH + 0.5 * TV);
			double NUM = 10.0 * (1.0 + 4.0 * JCONS) * (1.0 + 4.0 * JCONS);
			double DEN = (12.0 + 96.0 * JCONS + 276.0 * JCONS * JCONS + 192.0 * JCONS * JCONS * JCONS) + 30.0 * KCONS * KCONS * JCONS * (1.0 + JCONS);
			// J = It1 + It2 + beta3 * t2 * pow(t1, 3.0) / 2.0;
			double K1 = (5.0 / 6.0) * HV * TV / area;
			if (abs(DEN) > 0.0)
			{
				 K2 = NUM / DEN * area / area;
			}
			else
			{
				 K2 = 0.0;
			}
			_data.push_back(J);//抗扭截面系数
			//OA _data.push_back(shearCenter.Y());//剪切中心
			//OA _data.push_back(shearCenter.X());//剪切中心
			_data.push_back(K1);//截切系数
			_data.push_back(K2);//剪切系数



			////求抗扭截面系数
			//double ti1, bi1, Ci1 = 1.0 / 3.0;

			//bi1 = a;
			//ti1 = t2;

			//double It1 = Ci1 * bi1*pow(ti1, 3);

			//double ti2, bi2, Ci2 = 1.0 / 3.0;

			//bi2 = b - t2;
			//ti2 = t1;
			//double It2 = Ci2 * bi2*pow(ti2, 3);
			//double It = It1 + It2 + 1.0 / 3.0*t2*pow(t1, 3) / 2;
			//_data.push_back(It);
			//_data.push_back(shearCenter.Y());
			//_data.push_back(shearCenter.X());
			//double SFZ = (a - t1)*t2 / _data.at(0);
			//double SFY = (b - t2)*t1 / _data.at(0);
			//_data.push_back(SFY);
			//_data.push_back(SFZ);
			/*emit showDataOnPanel(_data);*/
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
		}
	
		
	}

	void T1_SectionVisual::ReBuildT1Sec(QList<double> para)
	{
		double area, Iz, Iy, J, Iyz, K1, K2;
		try
		{
			DIM1 = para.at(1);
			DIM2 = para.at(2);
			DIM3 = para.at(3);
			DIM4 = para.at(4);
			//_geoWindow->getAisContext()->EraseAll(true);
			para.removeFirst();
			_para = para;

			double _Y1 = DIM4 / 2, _X1 = -DIM2 / 2, _Z1 = 0;
			double _Y2 = DIM4 / 2, _X2 = DIM2 / 2, _Z2 = 0;
			double _Y3 = DIM1 / 2, _X3 = DIM2 / 2, _Z3 = 0;
			double _Y4 = DIM1 / 2, _X4 = DIM2 / 2 + DIM3, _Z4 = 0;

			//OA gp_Pnt Point1(_X1, _Y1, _Z1); gp_Pnt Point8(_X1, -_Y1, _Z1);
			//OA gp_Pnt Point2(_X2, _Y2, _Z2); gp_Pnt Point7(_X2, -_Y2, _Z2);
			//OA gp_Pnt Point3(_X3, _Y3, _Z3); gp_Pnt Point6(_X3, -_Y3, _Z3);
			//OA gp_Pnt Point4(_X4, _Y4, _Z4); gp_Pnt Point5(_X4, -_Y4, _Z4);


			//OA gp_Pnt Point27(_X2, 0, _Z3); gp_Pnt Point34((_X3 + _X4) / 2.0, _Y4 + 0.5, _Z3); gp_Pnt Point45(_X4 + 0.5, 0, _Z3); gp_Pnt PointE68(0, -_Y3 - 0.5, _Z3);
			//OA gp_Pnt shearCenter(DIM2 / 2, 0, _Z3);
			//OA TopoDS_Edge Edge1 = BRepBuilderAPI_MakeEdge(Point1, Point2);
			//OA TopoDS_Edge Edge2 = BRepBuilderAPI_MakeEdge(Point2, Point3);
			//OA TopoDS_Edge Edge3 = BRepBuilderAPI_MakeEdge(Point3, Point4);
			//OA TopoDS_Edge Edge4 = BRepBuilderAPI_MakeEdge(Point4, Point5);
			//OA TopoDS_Edge Edge5 = BRepBuilderAPI_MakeEdge(Point5, Point6);
			//OA TopoDS_Edge Edge6 = BRepBuilderAPI_MakeEdge(Point6, Point7);
			//OA TopoDS_Edge Edge7 = BRepBuilderAPI_MakeEdge(Point7, Point8);
			//OA TopoDS_Edge Edge8 = BRepBuilderAPI_MakeEdge(Point8, Point1);
			//OA auto TSecWire1 = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//OA TopoDS_Wire TSecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6, Edge7, Edge8);
			//OA TSecWire1.Add(TSecWire2);
			//OA TopoDS_Shape TSecFace = BRepBuilderAPI_MakeFace(TSecWire1);
			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(TSecFace);
			//OA _aisshape = aisshape;
			//OA BaseshowBeamSec(aisshape);
			QString name = mPBeam::getInstance()->_nameParaListPair.key(getPara());;

			QList<QString> NAMElist = beamView::_nameAisPair.keys();
			if (!NAMElist.contains(name))
			{
				/*emit AddSecSignal(name);
				emit showParaOnPanel(_para);*/
			}
			else
			{
				beamView::_nameAisPair.remove(name);
				mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());
			}
			//OA beamView::_nameAisPair.insert(name, aisshape);
			//OA beamView::_secNameAisPair.insert(name, aisshape);
			mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());
			//OA getDataFromBeam(name, TSecFace);
			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(shearCenter.X(), shearCenter.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//_geoWindow->getAisContext()->Display(shearMarker, Standard_False);
			//OA beamView::_nameAisPair.insert(name, shearMarker);
			//OA _shearMarker = shearMarker;

			//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();


			//OA Handle(AIS_LengthDimension) a45LenDim = new AIS_LengthDimension(Edge4, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an45Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString aValue = a45LenDim->GetValue();
			//OA TCollection_AsciiString atext = "DIM1 = ";
			//OA an45Aspect->SetValueStringFormat(atext + aValue);
			//OA an45Aspect->MakeArrows3d(Standard_False);
			//OA an45Aspect->MakeText3d(Standard_False);
			//OA an45Aspect->TextAspect()->SetHeight(0.3);
			//OA an45Aspect->ArrowAspect()->SetLength(0.2);
			//OA an45Aspect->MakeTextShaded(Standard_True);
			//OA an45Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an45Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an45Aspect->SetArrowTailSize(0.3);
			//OA an45Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a45LenDim->SetTextPosition(Point45);
			//OA a45LenDim->SetDimensionAspect(an45Aspect);
			//_geoWindow->getAisContext()->Display(a45LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a45LenDim);
			//OA _a45LenDim = a45LenDim;

			//OA Handle(AIS_LengthDimension) E68LenDim = new AIS_LengthDimension(Edge6, Edge8, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an68Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValue = E68LenDim->GetValue();
			//OA TCollection_AsciiString btext = "DIM2 = ";
			//OA an68Aspect->SetValueStringFormat(btext + bValue);
			//OA an68Aspect->MakeArrows3d(Standard_False);
			//OA an68Aspect->MakeText3d(Standard_False);
			//OA an68Aspect->TextAspect()->SetHeight(0.3);
			//OA an68Aspect->ArrowAspect()->SetLength(0.2);
			//OA an68Aspect->MakeTextShaded(Standard_True);
			//OA an68Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an68Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an68Aspect->SetArrowTailSize(0.3);
			//OA an68Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA E68LenDim->SetTextPosition(PointE68);
			//OA E68LenDim->SetDimensionAspect(an68Aspect);
			//_geoWindow->getAisContext()->Display(E68LenDim, true);
			//OA beamView::_nameAisPair.insert(name, E68LenDim);
			//OA _E68LenDim = E68LenDim;

			//OA Handle(AIS_LengthDimension) a34LenDim = new AIS_LengthDimension(Edge3, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an34Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t2Value = a34LenDim->GetValue();
			//OA TCollection_AsciiString t2text = "DIM3 = ";
			//OA an34Aspect->SetValueStringFormat(t2text + t2Value);
			//OA an34Aspect->MakeArrows3d(Standard_False);
			//OA an34Aspect->MakeText3d(Standard_False);
			//OA an34Aspect->TextAspect()->SetHeight(0.3);
			//OA an34Aspect->ArrowAspect()->SetLength(0.2);
			//OA an34Aspect->MakeTextShaded(Standard_True);
			//OA an34Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an34Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an34Aspect->SetArrowTailSize(0.3);
			//OA an34Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a34LenDim->SetTextPosition(Point34);
			//OA a34LenDim->SetDimensionAspect(an34Aspect);
			//_geoWindow->getAisContext()->Display(a34LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a34LenDim);
			//OA _a34LenDim = a34LenDim;

			//OA Handle(AIS_LengthDimension) a27LenDim = new AIS_LengthDimension(Point2, Point7, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an27Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t1Value = a27LenDim->GetValue();
			//OA TCollection_AsciiString t1text = "DIM4 = ";
			//OA an27Aspect->SetValueStringFormat(t1text + t1Value);
			//OA an27Aspect->MakeArrows3d(Standard_False);
			//OA an27Aspect->MakeText3d(Standard_False);
			//OA an27Aspect->TextAspect()->SetHeight(0.3);
			//OA an27Aspect->ArrowAspect()->SetLength(0.2);
			//OA an27Aspect->MakeTextShaded(Standard_True);
			//OA an27Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an27Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an27Aspect->SetArrowTailSize(0.3);
			//OA an27Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a27LenDim->SetTextPosition(Point27);
			//OA a27LenDim->SetDimensionAspect(an27Aspect);
			//_geoWindow->getAisContext()->Display(a27LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a27LenDim);
			//OA _a27LenDim = a27LenDim;


			double a = DIM1;
			double b = DIM2 + DIM3;
			double t1 = DIM4;
			double t2 = DIM3;
			area = a * t2 + (b - t2) * t1;
			double Y = ((a - t1) * t2 * (b / 2.0 - t2 / 2.0) + 0) / area;
			double Iz = (t2 * pow(a, 3) + (b - t2) * pow(t1, 3)) / 12.0;
			double S1 = (a - t1) * t2, Y1 = b / 2.0 - t2 / 2.0,
				I1Z = (a - t1) * pow(t2, 3) / 12.0 + S1 * (Y - Y1) * (Y - Y1),
				S2 = b * t1, Z2 = 0, Y2 = 0,
				I2Z = t1 * pow(b, 3) / 12.0 + S2 * Y * Y;
			Iy = I1Z + I2Z;
			Iyz = 0;
			// double beta1 = gettorsioncoefficient(a, t2);
			// double beta2 = gettorsioncoefficient(b - t2, t1);
			// double beta3 = gettorsioncoefficient(t2, t1);
			// double It1 = beta1 * a * pow(t2, 3.0),
			//        It2 = beta2 * (b - t2) * pow(t1, 3.0);
			double HV = a; double TV = t2; double WH = b - t2; double TH = t1;
			double J = 1.0 / 3.0 * (WH * TH * TH * TH + HV * TV * TV * TV);
			double JCONS = HV * TV / ((WH + 0.5 * TV) * TH);
			double KCONS = HV / (WH + 0.5 * TV);
			double NUM = 10.0 * (1.0 + 4.0 * JCONS) * (1.0 + 4.0 * JCONS);
			double DEN = (12.0 + 96.0 * JCONS + 276.0 * JCONS * JCONS + 192.0 * JCONS * JCONS * JCONS) + 30.0 * KCONS * KCONS * JCONS * (1.0 + JCONS);
			// J = It1 + It2 + beta3 * t2 * pow(t1, 3.0) / 2.0;
			double K1 = (5.0 / 6.0) * HV * TV / area;
			if (abs(DEN) > 0.0)
			{
				K2 = NUM / DEN * area / area;
			}
			else
			{
				K2 = 0.0;
			}
			_data.push_back(J);//抗扭截面系数
			//OA _data.push_back(shearCenter.Y());//剪切中心
			//OA _data.push_back(shearCenter.X());//剪切中心
			_data.push_back(K1);//截切系数
			_data.push_back(K2);//剪切系数



			////求抗扭截面系数
			//double ti1, bi1, Ci1 = 1.0 / 3.0;

			//bi1 = a;
			//ti1 = t2;

			//double It1 = Ci1 * bi1*pow(ti1, 3);

			//double ti2, bi2, Ci2 = 1.0 / 3.0;

			//bi2 = b - t2;
			//ti2 = t1;
			//double It2 = Ci2 * bi2*pow(ti2, 3);
			//double It = It1 + It2 + 1.0 / 3.0*t2*pow(t1, 3) / 2;
			//_data.push_back(It);
			//_data.push_back(shearCenter.Y());
			//_data.push_back(shearCenter.X());
			//double SFZ = (a - t1)*t2 / _data.at(0);
			//double SFY = (b - t2)*t1 / _data.at(0);
			//_data.push_back(SFY);
			//_data.push_back(SFZ);
			/*emit showDataOnPanel(_data);*/
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
		}
		
	}
}

