//#include "Polygon_SectionVisual.h"
//#include "mPBeam.h"
//#include "gp_Pnt.hxx"
//#include "gp_Lin.hxx"
//#include "gp_Dir.hxx"
//#include <BRepBuilderAPI_MakeEdge.hxx>
//#include <BRepBuilderAPI_MakePolygon.hxx>
//#include <BRepBuilderAPI_MakeFace.hxx>
//#include <TopoDS_Wire.hxx>
//#include <BRepAlgoAPI_Cut.hxx>
//#include <gp_Circ.hxx>
//#include "gp_Pnt.hxx"
//#include "BRepBuilderAPI_MakeEdge.hxx"
//#include "BRepBuilderAPI_MakeFace.hxx"
//#include "BRepBuilderAPI_MakeWire.hxx"
//#include "TopoDS_Face.hxx"
//#include "TopoDS_Wire.hxx"
//#include "gp_Circ.hxx"
//#include "gp_Torus.hxx"
//#include "BRepOffsetAPI_MakeOffset.hxx"
//#include "BRepAlgoAPI_Cut.hxx"
//
//#include <Prs3d_Drawer.hxx>
//#include <AIS_InteractiveContext.hxx>
//#include <AIS_LengthDimension.hxx>
////#include "GeoWidget.h"
//#include "GC_MakePlane.hxx"
//#include "AIS_RadiusDimension.hxx"
//#include "Sample2D_Markers.h"
//#include <BRepGProp.hxx>
//#include <GProp_GProps.hxx>
//#include <BRep_Tool.hxx>
//#include <math.h>
//#include <mGlobalSignals.h>
//
////#include "beamView.h"
//
//namespace MechSectionShapeData
//{
//	Polygon_SectionVisual::Polygon_SectionVisual(QWidget*m, MxBeam::GeoWidget*g)
//		: BeamSectionVisualBase(m,g)
//	{
//		_geoWindow = g;
//		_para.push_back((double) Sec_Polygon);
//		_para.push_back(_R);
//		_para.push_back(_N);
//		_para.push_back(_T);
//	}
//
//	Polygon_SectionVisual::~Polygon_SectionVisual()
//	{
//	}
//
//	void Polygon_SectionVisual::CreatePolygonSec()
//	{
//		try
//		{
//			gp_Pnt ReferPoint(0, 0, 0);
//			gp_Dir Dir(0, 0, 1);
//			gp_Ax1  ReferAX(ReferPoint, Dir);
//			gp_Dir ReferDir(0, 1, 0);
//			double  ang = 2.0* PI / _N;
//			_para.push_back(ang);
//			gp_Lin  RerferLine(ReferPoint, ReferDir);
//			gp_Lin  RerferLine1 = RerferLine.Rotated(ReferAX, ang);
//			gp_Lin  RerferLine2 = RerferLine1.Rotated(ReferAX, ang);
//			gp_Lin  RerferLine3 = RerferLine2.Rotated(ReferAX, ang);
//			gp_Lin  RerferLine4 = RerferLine3.Rotated(ReferAX, ang);
//			gp_Lin  RerferLine5 = RerferLine4.Rotated(ReferAX, ang);
//			gp_Lin  RerferLine6 = RerferLine5.Rotated(ReferAX, ang);
//			auto edge1 = BRepBuilderAPI_MakeEdge(RerferLine1, _R - _T / (sin(ang)), _R);
//			auto edge2 = BRepBuilderAPI_MakeEdge(RerferLine2, _R - _T / (sin(ang)), _R);
//			auto edge3 = BRepBuilderAPI_MakeEdge(RerferLine3, _R - _T / (sin(ang)), _R);
//			auto edge4 = BRepBuilderAPI_MakeEdge(RerferLine4, _R - _T / (sin(ang)), _R);
//			auto edge5 = BRepBuilderAPI_MakeEdge(RerferLine5, _R - _T / (sin(ang)), _R);
//			auto edge6 = BRepBuilderAPI_MakeEdge(RerferLine6, _R - _T / (sin(ang)), _R);
//
//			auto P1 = edge1.Vertex2();
//			auto P2 = edge2.Vertex2();
//			auto P3 = edge3.Vertex2();
//			auto P4 = edge4.Vertex2();
//			auto P5 = edge5.Vertex2();
//			auto P6 = edge6.Vertex2();
//
//			gp_Pnt PP1 = BRep_Tool::Pnt(P5);
//			gp_Pnt PP2 = BRep_Tool::Pnt(P2);
//			gp_Pnt PP3 = BRep_Tool::Pnt(P1);
//			gp_Pnt Point666(PP3.X() + _T / 2, PP3.Y() + 3, 0);
//			BRepBuilderAPI_MakePolygon aPoly1;
//			aPoly1.Add(P1);
//			aPoly1.Add(P2);
//			aPoly1.Add(P3);
//			aPoly1.Add(P4);
//			aPoly1.Add(P5);
//			aPoly1.Add(P6);
//			aPoly1.Add(P1);
//			TopoDS_Wire aWire = aPoly1;
//			BRepBuilderAPI_MakeFace aface(aWire);
//
//			auto P11 = edge1.Vertex1();
//			auto P22 = edge2.Vertex1();
//			auto P33 = edge3.Vertex1();
//			auto P44 = edge4.Vertex1();
//			auto P55 = edge5.Vertex1();
//			auto P66 = edge6.Vertex1();
//			BRepBuilderAPI_MakePolygon aPoly2;
//			aPoly2.Add(P11);
//			aPoly2.Add(P22);
//			aPoly2.Add(P33);
//			aPoly2.Add(P44);
//			aPoly2.Add(P55);
//			aPoly2.Add(P66);
//			aPoly2.Add(P11);
//			TopoDS_Wire aWire2 = aPoly2;
//			BRepBuilderAPI_MakeFace aface1(aWire2);
//			gp_Circ circle(gp_Ax2(ReferPoint, Dir), _R);
//			BRepBuilderAPI_MakeEdge cir(circle);
//			Handle(AIS_Shape) aisshape2 = new AIS_Shape(cir);
//
//			TopoDS_Shape PolySecFace = BRepAlgoAPI_Cut(aface, aface1);
//			Handle(AIS_Shape) aisshape1 = new AIS_Shape(PolySecFace);
//			BaseshowBeamSec(aisshape1);
//			BaseshowBeamSec(aisshape2);
//
//			QString name = mPBeam::getInstance()->setDefaultNameForBeam(Sec_Polygon);
//			emit AddSecSignal(name);
//			emit showParaOnPanel(_para, name);
//			//beamView::_nameAisPair.insert(name, aisshape1);
//			//beamView::_nameAisPair.insert(name, aisshape2);
//			//beamView::_secNameAisPair.insert(name, aisshape1);
//			//beamView::_secNameAisPair.insert(name, aisshape2);
//			mPBeam::getInstance()->_nameParaListPair.insert(name, _para);
//
//			getDataFromBeam(name, PolySecFace);
//			Handle(AIS_RadiusDimension) a14LenDim = new AIS_RadiusDimension(circle, PP1);
//			Handle(Prs3d_DimensionAspect) an14Aspect = new Prs3d_DimensionAspect();
//
//			TCollection_AsciiString aValue = a14LenDim->GetValue();
//
//			an14Aspect->SetValueStringFormat(aValue);
//			an14Aspect->MakeArrows3d(Standard_False);
//			an14Aspect->MakeText3d(Standard_False);
//			an14Aspect->TextAspect()->SetHeight(0.3);
//			an14Aspect->ArrowAspect()->SetLength(0.8);
//			an14Aspect->MakeTextShaded(Standard_True);
//			an14Aspect->SetCommonColor(Quantity_NOC_BLACK);
//			an14Aspect->MakeUnitsDisplayed(Standard_False);
//			an14Aspect->SetArrowTailSize(0.3);
//			an14Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
//			a14LenDim->SetDimensionAspect(an14Aspect);
//			//_geoWindow->getAisContext()->Display(a14LenDim, true);
//			//beamView::_nameAisPair.insert("name", a14LenDim);
//
//			GC_MakePlane aMkPlane(PP1, PP2, PP3);
//			Handle(Geom_Plane) aPlane = aMkPlane.Value();
//
//			auto edge7 = BRepBuilderAPI_MakeEdge(P1, P2);
//			auto edge8 = BRepBuilderAPI_MakeEdge(P11, P22);
//			Handle(AIS_LengthDimension) a25LenDim = new AIS_LengthDimension(edge7, edge8, aPlane->Pln());
//			Handle(Prs3d_DimensionAspect) an25Aspect = new Prs3d_DimensionAspect();
//			an25Aspect->ArrowAspect()->SetLength(0.2);
//			TCollection_AsciiString tValue = a25LenDim->GetValue();
//			TCollection_AsciiString ttext = "t = ";
//			an25Aspect->SetValueStringFormat(ttext + tValue);
//			an25Aspect->MakeArrows3d(Standard_False);
//			an25Aspect->MakeText3d(Standard_False);
//			an25Aspect->TextAspect()->SetHeight(0.3);
//			an25Aspect->MakeTextShaded(Standard_True);
//			an25Aspect->SetCommonColor(Quantity_NOC_BLACK);
//			an25Aspect->MakeUnitsDisplayed(Standard_False);
//			an25Aspect->SetArrowTailSize(0.3);
//			an25Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
//			a25LenDim->SetTextPosition(Point666);
//			a25LenDim->SetDimensionAspect(an25Aspect);
//			//_geoWindow->getAisContext()->Display(a25LenDim, true);
//			//beamView::_nameAisPair.insert(name, a25LenDim);
//			GProp_GProps System;
//			BRepGProp::SurfaceProperties(PolySecFace, System);
//			auto CentreOfMass = System.CentreOfMass();
//			Handle(Sample2D_Markers) shearMarker =
//				new Sample2D_Markers(CentreOfMass.X(), CentreOfMass.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
//			//_geoWindow->getAisContext()->Display(shearMarker, true);
//			//beamView::_nameAisPair.insert(name, shearMarker);
//			//求抗扭截面系数
//
//			Handle(AIS_LengthDimension) aLenDim1 = new AIS_LengthDimension(P1, P2, aPlane->Pln());
//			auto Length1 = aLenDim1->GetValue();
//			Handle(AIS_LengthDimension) aLenDim2 = new AIS_LengthDimension(P11, P22, aPlane->Pln());
//			auto Length2 = aLenDim2->GetValue();
//			double Length = (Length1 + Length2) / 2;
//			double S = _N * Length*Length*sin(ang) / 4.0 / (1 - cos(ang));
//			double It = 4.0 * S*S / (Length*_N / _T);
//			_data.push_back(It);
//			_data.push_back(CentreOfMass.Y());
//			_data.push_back(CentreOfMass.X());
//			_data.push_back(0);
//			_data.push_back(0);
//			emit showDataOnPanel(_data);
//			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
//		}
//		catch (...)
//		{
//			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
//		}
//		
//	}
//
//	//void Polygon_SectionVisual::ReBulidPolygonSec(QList<double> para)
//	//{
//	//	try
//	//	{
//	//		_geoWindow->getAisContext()->EraseAll(true);
//	//		_para = para;
//	//		_R = _para.at(1);
//	//		_N = _para.at(2);
//	//		_T = _para.at(3);
//	//		double  ang = 2 * PI / _N;
//	//		_para.push_back(ang);
//	//		gp_Pnt ReferPoint(0, 0, 0);
//	//		gp_Dir Dir(0, 0, 1);
//	//		gp_Ax1  ReferAX(ReferPoint, Dir);
//	//		gp_Dir ReferDir(0, 1, 0);
//	//		gp_Lin  RerferLine(ReferPoint, ReferDir);
//	//		BRepBuilderAPI_MakePolygon aPoly1;
//	//		BRepBuilderAPI_MakePolygon aPoly2;
//
//	//		for (int i = 0; i < _N; i++)
//	//		{
//
//	//			auto edge = BRepBuilderAPI_MakeEdge(RerferLine, _R - _T / (sin((PI - ang) / 2)), _R);
//	//			auto P1 = edge.Vertex1();
//	//			auto P2 = edge.Vertex2();
//	//			aPoly1.Add(P1);
//	//			_PointINTList.push_back(P1);
//	//			aPoly2.Add(P2);
//	//			_PointEXTList.push_back(P2);
//	//			gp_Lin  RerferLine1 = RerferLine.Rotated(ReferAX, ang);
//	//			RerferLine = RerferLine1;
//	//		}
//	//		aPoly1.Add(_PointINTList.first());
//	//		aPoly2.Add(_PointEXTList.first());
//	//		TopoDS_Wire aWire = aPoly1;
//	//		BRepBuilderAPI_MakeFace aface(aWire);
//
//	//		TopoDS_Wire aWire2 = aPoly2;
//	//		BRepBuilderAPI_MakeFace aface1(aWire2);
//
//	//		TopoDS_Shape PolySecFace = BRepAlgoAPI_Cut(aface1, aface);
//	//		Handle(AIS_Shape) aisshape1 = new AIS_Shape(PolySecFace);
//	//		BaseshowBeamSec(aisshape1);
//	//		gp_Circ circle(gp_Ax2(ReferPoint, Dir), _R);
//	//		BRepBuilderAPI_MakeEdge cir(circle);
//	//		Handle(AIS_Shape) aisshape2 = new AIS_Shape(cir);
//	//		BaseshowBeamSec(aisshape2);
//
//	//		QString name = mPBeam::getInstance()->_nameParaListPair.key(_para);
//
//	//		QList<QString> NAMElist = beamView::_nameAisPair.keys();
//	//		if (!NAMElist.contains(name))
//	//		{
//	//			emit AddSecSignal(name);
//	//			emit showParaOnPanel(_para);
//	//		}
//	//		else
//	//		{
//	//			beamView::_nameAisPair.remove(name);
//	//			mPBeam::getInstance()->_nameParaListPair.insert(name, _para);
//	//		}
//
//	//		beamView::_nameAisPair.insert(name, aisshape1);
//	//		beamView::_nameAisPair.insert(name, aisshape2);
//	//		beamView::_secNameAisPair.insert(name, aisshape1);
//	//		beamView::_secNameAisPair.insert(name, aisshape2);
//
//	//		auto P1 = _PointINTList.at(0);
//	//		auto P2 = _PointINTList.at(1);
//	//		auto P3 = _PointINTList.at(2);
//	//		auto P11 = _PointEXTList.at(0);
//	//		auto P22 = _PointEXTList.at(1);
//	//		auto P33 = _PointEXTList.at(2);
//	//		gp_Pnt PP1 = BRep_Tool::Pnt(P1);
//	//		gp_Pnt PP2 = BRep_Tool::Pnt(P2);
//	//		gp_Pnt PP3 = BRep_Tool::Pnt(P3);
//	//		gp_Pnt PP11 = BRep_Tool::Pnt(P11);
//	//		getDataFromBeam(name, PolySecFace);
//	//		Handle(AIS_RadiusDimension) a14LenDim = new AIS_RadiusDimension(circle, PP11);
//	//		Handle(Prs3d_DimensionAspect) an14Aspect = new Prs3d_DimensionAspect();
//
//	//		TCollection_AsciiString aValue = a14LenDim->GetValue();
//
//	//		an14Aspect->SetValueStringFormat(aValue);
//	//		an14Aspect->MakeArrows3d(Standard_False);
//	//		an14Aspect->MakeText3d(Standard_False);
//	//		an14Aspect->TextAspect()->SetHeight(0.3);
//	//		an14Aspect->ArrowAspect()->SetLength(0.8);
//	//		an14Aspect->MakeTextShaded(Standard_True);
//	//		an14Aspect->SetCommonColor(Quantity_NOC_BLACK);
//	//		an14Aspect->MakeUnitsDisplayed(Standard_False);
//	//		an14Aspect->SetArrowTailSize(0.3);
//	//		an14Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
//	//		a14LenDim->SetDimensionAspect(an14Aspect);
//	//		_geoWindow->getAisContext()->Display(a14LenDim, true);
//	//		beamView::_nameAisPair.insert("name", a14LenDim);
//	//		gp_Pnt Point666(PP2.X() + _T / 2, PP2.Y() + 3, 0);
//	//		GC_MakePlane aMkPlane(PP1, PP2, PP3);
//	//		Handle(Geom_Plane) aPlane = aMkPlane.Value();
//
//	//		auto edge7 = BRepBuilderAPI_MakeEdge(P2, P3);
//	//		auto edge8 = BRepBuilderAPI_MakeEdge(P22, P33);
//	//		Handle(AIS_LengthDimension) a25LenDim = new AIS_LengthDimension(edge7, edge8, aPlane->Pln());
//	//		Handle(Prs3d_DimensionAspect) an25Aspect = new Prs3d_DimensionAspect();
//	//		an25Aspect->ArrowAspect()->SetLength(0.2);
//	//		TCollection_AsciiString tValue = a25LenDim->GetValue();
//	//		TCollection_AsciiString ttext = "t = ";
//	//		an25Aspect->SetValueStringFormat(ttext + tValue);
//	//		an25Aspect->MakeArrows3d(Standard_False);
//	//		an25Aspect->MakeText3d(Standard_False);
//	//		an25Aspect->TextAspect()->SetHeight(0.3);
//	//		an25Aspect->MakeTextShaded(Standard_True);
//	//		an25Aspect->SetCommonColor(Quantity_NOC_BLACK);
//	//		an25Aspect->MakeUnitsDisplayed(Standard_False);
//	//		an25Aspect->SetArrowTailSize(0.3);
//	//		an25Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
//	//		a25LenDim->SetTextPosition(Point666);
//	//		a25LenDim->SetDimensionAspect(an25Aspect);
//	//		_geoWindow->getAisContext()->Display(a25LenDim, true);
//	//		beamView::_nameAisPair.insert(name, a25LenDim);
//	//		GProp_GProps System;
//	//		BRepGProp::SurfaceProperties(PolySecFace, System);
//	//		auto CentreOfMass = System.CentreOfMass();
//	//		Handle(Sample2D_Markers) shearMarker =
//	//			new Sample2D_Markers(CentreOfMass.X(), CentreOfMass.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
//	//		_geoWindow->getAisContext()->Display(shearMarker, true);
//	//		beamView::_nameAisPair.insert(name, shearMarker);
//	//		//求抗扭截面系数
//
//	//		Handle(AIS_LengthDimension) aLenDim1 = new AIS_LengthDimension(P1, P2, aPlane->Pln());
//	//		auto Length1 = aLenDim1->GetValue();
//	//		Handle(AIS_LengthDimension) aLenDim2 = new AIS_LengthDimension(P11, P22, aPlane->Pln());
//	//		auto Length2 = aLenDim2->GetValue();
//	//		double Length = (Length1 + Length2) / 2;
//	//		double S = _N * Length*Length*sin(ang) / 4.0 / (1 - cos(ang));
//	//		double It = 4.0 * S *S / (Length*_N / _T);
//	//		_data.push_back(It);
//	//		_data.push_back(CentreOfMass.Y());
//	//		_data.push_back(CentreOfMass.X());
//	//		_data.push_back(0);
//	//		_data.push_back(0);
//	//		emit showDataOnPanel(_data);
//	//		mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
//	//	}
//	//	catch (...)
//	//	{
//	//		MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
//	//	}
//	//	
//	//}
//}
//
