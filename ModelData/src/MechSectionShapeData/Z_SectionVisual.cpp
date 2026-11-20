#include "Z_SectionVisual.h"
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
	Z_SectionVisual::Z_SectionVisual(QString name, int id)
		:MechSectionShapeData::mSectionShapeBase(Sec_Z,name,id)
	{
		_para.push_back(DIM1);
		_para.push_back(DIM2);
		_para.push_back(DIM3);
		_para.push_back(DIM4);
	}

	Z_SectionVisual::~Z_SectionVisual()
	{
	}
	Z_SectionVisual* Z_SectionVisual::clone() const
	{
		Z_SectionVisual* newSec = new Z_SectionVisual(_sectionShapeName, _id);

		newSec->DIM1 = DIM1;
		newSec->DIM2 = DIM2;
		newSec->DIM3 = DIM3;
		newSec->DIM4 = DIM4;

		newSec->_shearMarker = _shearMarker;
		newSec->_a12LenDim = _a12LenDim;
		newSec->_E27LenDim = _E27LenDim;
		newSec->_a3LenDim = _a3LenDim;
		newSec->_a15LenDim = _a15LenDim;
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
	void Z_SectionVisual::CreateZSec()
	{
		try
		{

			double _Y1 = DIM4 / 2, _X1 = -DIM1 - DIM2/2.0, _Z1 = 0;
			double _Y2 = DIM4 / 2, _X2 = DIM2 / 2, _Z2 = 0;
			double _Y3 = -DIM3 / 2, _X3 = DIM2 / 2, _Z3 = 0;
			double _Y4 = -DIM3 / 2, _X4 = DIM1 + DIM2 / 2.0, _Z4 = 0;
			double _Y5 = -DIM4 / 2.0, _X5 = DIM1 + DIM2 / 2.0, _Z5 = 0;
			double _Y6 = -DIM4 / 2.0, _X6 = -DIM2 / 2.0, _Z6 = 0;
			double _Y7 = DIM3 / 2.0, _X7 = -DIM2 / 2.0, _Z7 = 0;
			double _Y8 = DIM3 / 2.0, _X8 = -DIM1 - DIM2 / 2.0, _Z8 = 0;

			//OA gp_Pnt Point1(_X1, _Y1, _Z1); gp_Pnt Point8(_X8, _Y8, _Z8);
			//OA gp_Pnt Point2(_X2, _Y2, _Z2); gp_Pnt Point7(_X7, _Y7, _Z7);
			//OA gp_Pnt Point3(_X3, _Y3, _Z3); gp_Pnt Point6(_X6, _Y6, _Z6);
			//OA gp_Pnt Point4(_X4, _Y4, _Z4); gp_Pnt Point5(_X5, _Y5, _Z5);

			//OA gp_Pnt Point12(-DIM1/2.0 - DIM2 / 2.0, _Y2+0.5, _Z3);
			//OA gp_Pnt Point12End( - DIM2 / 2.0, _Y2 , _Z3);

			//OA gp_Pnt DIM2End(DIM2 / 2.0, DIM3 / 2.0, _Z3);
			//OA gp_Pnt DIM2Pos(0, DIM3 / 2.0, _Z3);

			//OA gp_Pnt Dim3Pos(DIM2 / 2.0 +0.5, 0, _Z3);

			//OA gp_Pnt Point15Pos(-DIM1 - DIM2 / 2.0-0.5, 0, _Z3);
			
			//OA gp_Pnt shearCenter(0, 0, _Z3);
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
			QString name = mPBeam::getInstance()->setDefaultNameForBeam(Sec_Z);

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


			//OA Handle(AIS_LengthDimension) a12LenDim = new AIS_LengthDimension(Point1, Point12End, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an12Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString aValue = a12LenDim->GetValue();
			//OA TCollection_AsciiString atext = "DIM1 = ";
			//OA an12Aspect->SetValueStringFormat(atext + aValue);
			//OA an12Aspect->MakeArrows3d(Standard_False);
			//OA an12Aspect->MakeText3d(Standard_False);
			//OA an12Aspect->TextAspect()->SetHeight(0.3);
			//OA an12Aspect->ArrowAspect()->SetLength(0.2);
			//OA an12Aspect->MakeTextShaded(Standard_True);
			//OA an12Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an12Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an12Aspect->SetArrowTailSize(0.3);
			//OA an12Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a12LenDim->SetTextPosition(Point12);
			//OA a12LenDim->SetDimensionAspect(an12Aspect);
			//_geoWindow->getAisContext()->Display(a12LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a12LenDim);
			//OA _a12LenDim = a12LenDim;

			//OA Handle(AIS_LengthDimension) E27LenDim = new AIS_LengthDimension(Point7, DIM2End, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an27Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValue = E27LenDim->GetValue();
			//OA TCollection_AsciiString btext = "DIM2 = ";
			//OA an27Aspect->SetValueStringFormat(btext + bValue);
			//OA an27Aspect->MakeArrows3d(Standard_False);
			//OA an27Aspect->MakeText3d(Standard_False);
			//OA an27Aspect->TextAspect()->SetHeight(0.3);
			//OA an27Aspect->ArrowAspect()->SetLength(0.2);
			//OA an27Aspect->MakeTextShaded(Standard_True);
			//OA an27Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an27Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an27Aspect->SetArrowTailSize(0.3);
			//OA an27Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA E27LenDim->SetTextPosition(DIM2Pos);
			//OA E27LenDim->SetDimensionAspect(an27Aspect);
			//_geoWindow->getAisContext()->Display(E27LenDim, true);
			//OA beamView::_nameAisPair.insert(name, E27LenDim);
			//OA _E27LenDim = E27LenDim;

			//OA Handle(AIS_LengthDimension) a3LenDim = new AIS_LengthDimension(DIM2End, Point3, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an3Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t2Value = a3LenDim->GetValue();
			//OA TCollection_AsciiString t2text = "DIM3 = ";
			//OA an3Aspect->SetValueStringFormat(t2text + t2Value);
			//OA an3Aspect->MakeArrows3d(Standard_False);
			//OA an3Aspect->MakeText3d(Standard_False);
			//OA an3Aspect->TextAspect()->SetHeight(0.3);
			//OA an3Aspect->ArrowAspect()->SetLength(0.2);
			//OA an3Aspect->MakeTextShaded(Standard_True);
			//OA an3Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an3Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an3Aspect->SetArrowTailSize(0.3);
			//OA an3Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a3LenDim->SetTextPosition(Dim3Pos);
			//OA a3LenDim->SetDimensionAspect(an3Aspect);
			//_geoWindow->getAisContext()->Display(a3LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a3LenDim);
			//OA _a3LenDim = a3LenDim;

			//OA Handle(AIS_LengthDimension) a15LenDim = new AIS_LengthDimension(Edge1, Edge5, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an15Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t1Value = a15LenDim->GetValue();
			//OA TCollection_AsciiString t1text = "DIM4 = ";
			//OA an15Aspect->SetValueStringFormat(t1text + t1Value);
			//OA an15Aspect->MakeArrows3d(Standard_False);
			//OA an15Aspect->MakeText3d(Standard_False);
			//OA an15Aspect->TextAspect()->SetHeight(0.3);
			//OA an15Aspect->ArrowAspect()->SetLength(0.2);
			//OA an15Aspect->MakeTextShaded(Standard_True);
			//OA an15Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an15Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an15Aspect->SetArrowTailSize(0.3);
			//OA an15Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a15LenDim->SetTextPosition(Point15Pos);
			//OA a15LenDim->SetDimensionAspect(an15Aspect);
			//_geoWindow->getAisContext()->Display(a15LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a15LenDim);
			//OA _a15LenDim = a15LenDim;

			double WF = DIM1 + DIM2;
			double TF = 0.5 * (DIM4 - DIM3);
			double HW = DIM3;
			double TW = DIM2;
			double ATOP = WF * TF;
			double ABOT = ATOP;
			double AWEB = HW * TW;
			area = ATOP + ABOT + AWEB;
			Iz = TW * HW * HW * HW / 12.0 + 2.0 * (WF * TF * TF * TF / 12.0 + WF * TF * 0.25 * (HW + TF) * (HW + TF));
			Iy = HW * TW * TW * TW / 12.0 + 2.0 * (TF * WF * WF * WF / 12.0 + TF * WF * 0.25 * (WF - TW) * (WF - TW));
			Iyz = -TF * DIM1 * (DIM1 + DIM2) * (DIM3 + TF) / 2.0;
			//double beta1 = gettorsioncoefficient(WF, TF);
			//double beta2 = gettorsioncoefficient(HW, TW);
			J = 1.0 / 3.0 * 2.0 * WF * TF * TF * TF + 1.0 / 3.0 * HW * TW * TW * TW;
			K1 = (5.0 / 6.0) * AWEB / area;
			K2 = (5.0 / 6.0) * 2.0 * DIM1 * TF / area;


			//求抗扭截面系数
			/*double ti1, bi1, Ci1 = 1.0 / 3.0;

			bi1 = a;
			ti1 = t2;

			double It1 = Ci1 * bi1*pow(ti1, 3);

			double ti2, bi2, Ci2 = 1.0 / 3.0;

			bi2 = b - t2;
			ti2 = t1;
			double It2 = Ci2 * bi2*pow(ti2, 3);
			double It = It1 + It2 + 1.0 / 3.0*t2*pow(t1, 3) / 2;
			_data.push_back(It);
			_data.push_back(shearCenter.Y());
			_data.push_back(shearCenter.X());
			double SFZ = (a - t1)*t2 / _data.at(0);
			double SFY = (b - t2)*t1 / _data.at(0);
			_data.push_back(SFY);
			_data.push_back(SFZ);*/
			_data.push_back(J);//抗扭截面系数
			//OA _data.push_back(shearCenter.Y());//剪切中心
			//OA _data.push_back(shearCenter.X());//剪切中心
			_data.push_back(K1);//截切系数
			_data.push_back(K2);//剪切系数
			//emit showDataOnPanel(_data);
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
		}
	
		
	}

	void Z_SectionVisual::ReBuildZSec(QList<double> para)
	{
		try
		{
			DIM1 = para.at(1);
			DIM2 = para.at(2);
			DIM3 = para.at(3);
			DIM4 = para.at(4);
			//_geoWindow->getAisContext()->EraseAll(true);
			para.removeFirst();
			_para = para;


			double _Y1 = DIM4 / 2, _X1 = -DIM1 - DIM2 / 2.0, _Z1 = 0;
			double _Y2 = DIM4 / 2, _X2 = DIM2 / 2, _Z2 = 0;
			double _Y3 = -DIM3 / 2, _X3 = DIM2 / 2, _Z3 = 0;
			double _Y4 = -DIM3 / 2, _X4 = DIM1 + DIM2 / 2.0, _Z4 = 0;
			double _Y5 = -DIM4 / 2.0, _X5 = DIM1 + DIM2 / 2.0, _Z5 = 0;
			double _Y6 = -DIM4 / 2.0, _X6 = -DIM2 / 2.0, _Z6 = 0;
			double _Y7 = DIM3 / 2.0, _X7 = -DIM2 / 2.0, _Z7 = 0;
			double _Y8 = DIM3 / 2.0, _X8 = -DIM1 - DIM2 / 2.0, _Z8 = 0;

			//OA gp_Pnt Point1(_X1, _Y1, _Z1); gp_Pnt Point8(_X8, _Y8, _Z8);
			//OA gp_Pnt Point2(_X2, _Y2, _Z2); gp_Pnt Point7(_X7, _Y7, _Z7);
			//OA gp_Pnt Point3(_X3, _Y3, _Z3); gp_Pnt Point6(_X6, _Y6, _Z6);
			//OA gp_Pnt Point4(_X4, _Y4, _Z4); gp_Pnt Point5(_X5, _Y5, _Z5);

			//OA gp_Pnt Point12(-DIM1 / 2.0 - DIM2 / 2.0, _Y2 + 0.5, _Z3);
			//OA gp_Pnt Point12End(-DIM2 / 2.0, _Y2, _Z3);

			//OA gp_Pnt DIM2End(DIM2 / 2.0, DIM3 / 2.0, _Z3);
			//OA gp_Pnt DIM2Pos(0, DIM3 / 2.0, _Z3);

			//OA gp_Pnt Dim3Pos(DIM2 / 2.0 + 0.5, 0, _Z3);

			//OA gp_Pnt Point15Pos(-DIM1 - DIM2 / 2.0 - 0.5, 0, _Z3);

			//OA gp_Pnt shearCenter(0, 0, _Z3);
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

			//OA getDataFromBeam(name, TSecFace);

			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(shearCenter.X(), shearCenter.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//_geoWindow->getAisContext()->Display(shearMarker, Standard_False);
			//OA beamView::_nameAisPair.insert(name, shearMarker);
			//OA _shearMarker = shearMarker;


			//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();




			//OA Handle(AIS_LengthDimension) a12LenDim = new AIS_LengthDimension(Point1, Point12End, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an12Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString aValue = a12LenDim->GetValue();
			//OA TCollection_AsciiString atext = "DIM1 = ";
			//OA an12Aspect->SetValueStringFormat(atext + aValue);
			//OA an12Aspect->MakeArrows3d(Standard_False);
			//OA an12Aspect->MakeText3d(Standard_False);
			//OA an12Aspect->TextAspect()->SetHeight(0.3);
			//OA an12Aspect->ArrowAspect()->SetLength(0.2);
			//OA an12Aspect->MakeTextShaded(Standard_True);
			//OA an12Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an12Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an12Aspect->SetArrowTailSize(0.3);
			//OA an12Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a12LenDim->SetTextPosition(Point12);
			//OA a12LenDim->SetDimensionAspect(an12Aspect);
			//_geoWindow->getAisContext()->Display(a12LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a12LenDim);
			//OA _a12LenDim = a12LenDim;

			//OA Handle(AIS_LengthDimension) E27LenDim = new AIS_LengthDimension(Point7, DIM2End, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an27Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValue = E27LenDim->GetValue();
			//OA TCollection_AsciiString btext = "DIM2 = ";
			//OA an27Aspect->SetValueStringFormat(btext + bValue);
			//OA an27Aspect->MakeArrows3d(Standard_False);
			//OA an27Aspect->MakeText3d(Standard_False);
			//OA an27Aspect->TextAspect()->SetHeight(0.3);
			//OA an27Aspect->ArrowAspect()->SetLength(0.2);
			//OA an27Aspect->MakeTextShaded(Standard_True);
			//OA an27Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an27Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an27Aspect->SetArrowTailSize(0.3);
			//OA an27Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA E27LenDim->SetTextPosition(DIM2Pos);
			//OA E27LenDim->SetDimensionAspect(an27Aspect);
			//_geoWindow->getAisContext()->Display(E27LenDim, true);
			//OA beamView::_nameAisPair.insert(name, E27LenDim);
			//OA _E27LenDim = E27LenDim;

			//OA Handle(AIS_LengthDimension) a3LenDim = new AIS_LengthDimension(DIM2End, Point3, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an3Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t2Value = a3LenDim->GetValue();
			//OA TCollection_AsciiString t2text = "DIM3 = ";
			//OA an3Aspect->SetValueStringFormat(t2text + t2Value);
			//OA an3Aspect->MakeArrows3d(Standard_False);
			//OA an3Aspect->MakeText3d(Standard_False);
			//OA an3Aspect->TextAspect()->SetHeight(0.3);
			//OA an3Aspect->ArrowAspect()->SetLength(0.2);
			//OA an3Aspect->MakeTextShaded(Standard_True);
			//OA an3Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an3Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an3Aspect->SetArrowTailSize(0.3);
			//OA an3Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a3LenDim->SetTextPosition(Dim3Pos);
			//OA a3LenDim->SetDimensionAspect(an3Aspect);
			//_geoWindow->getAisContext()->Display(a3LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a3LenDim);
			//OA _a3LenDim = a3LenDim;

			//OA Handle(AIS_LengthDimension) a15LenDim = new AIS_LengthDimension(Edge1, Edge5, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an15Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t1Value = a15LenDim->GetValue();
			//OA TCollection_AsciiString t1text = "DIM4 = ";
			//OA an15Aspect->SetValueStringFormat(t1text + t1Value);
			//OA an15Aspect->MakeArrows3d(Standard_False);
			//OA an15Aspect->MakeText3d(Standard_False);
			//OA an15Aspect->TextAspect()->SetHeight(0.3);
			//OA an15Aspect->ArrowAspect()->SetLength(0.2);
			//OA an15Aspect->MakeTextShaded(Standard_True);
			//OA an15Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an15Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an15Aspect->SetArrowTailSize(0.3);
			//OA an15Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a15LenDim->SetTextPosition(Point15Pos);
			//OA a15LenDim->SetDimensionAspect(an15Aspect);
			//_geoWindow->getAisContext()->Display(a15LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a15LenDim);
			//OA _a15LenDim = a15LenDim;

			double WF = DIM1 + DIM2;
			double TF = 0.5 * (DIM4 - DIM3);
			double HW = DIM3;
			double TW = DIM2;
			double ATOP = WF * TF;
			double ABOT = ATOP;
			double AWEB = HW * TW;
			area = ATOP + ABOT + AWEB;
			Iz = TW * HW * HW * HW / 12.0 + 2.0 * (WF * TF * TF * TF / 12.0 + WF * TF * 0.25 * (HW + TF) * (HW + TF));
			Iy = HW * TW * TW * TW / 12.0 + 2.0 * (TF * WF * WF * WF / 12.0 + TF * WF * 0.25 * (WF - TW) * (WF - TW));
			Iyz = -TF * DIM1 * (DIM1 + DIM2) * (DIM3 + TF) / 2.0;
			//double beta1 = gettorsioncoefficient(WF, TF);
			//double beta2 = gettorsioncoefficient(HW, TW);
			J = 1.0 / 3.0 * 2.0 * WF * TF * TF * TF + 1.0 / 3.0 * HW * TW * TW * TW;
			K1 = (5.0 / 6.0) * AWEB / area;
			K2 = (5.0 / 6.0) * 2.0 * DIM1 * TF / area;


			//求抗扭截面系数
			/*double ti1, bi1, Ci1 = 1.0 / 3.0;

			bi1 = a;
			ti1 = t2;

			double It1 = Ci1 * bi1*pow(ti1, 3);

			double ti2, bi2, Ci2 = 1.0 / 3.0;

			bi2 = b - t2;
			ti2 = t1;
			double It2 = Ci2 * bi2*pow(ti2, 3);
			double It = It1 + It2 + 1.0 / 3.0*t2*pow(t1, 3) / 2;
			_data.push_back(It);
			_data.push_back(shearCenter.Y());
			_data.push_back(shearCenter.X());
			double SFZ = (a - t1)*t2 / _data.at(0);
			double SFY = (b - t2)*t1 / _data.at(0);
			_data.push_back(SFY);
			_data.push_back(SFZ);*/
			_data.push_back(J);//抗扭截面系数
			//OA _data.push_back(shearCenter.Y());//剪切中心
			//OA _data.push_back(shearCenter.X());//剪切中心
			_data.push_back(K1);//截切系数
			_data.push_back(K2);//剪切系数
			//emit showDataOnPanel(_data);
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
		}
		
	}
}

