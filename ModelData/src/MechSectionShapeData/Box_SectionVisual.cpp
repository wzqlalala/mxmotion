#include "Box_SectionVisual.h"
//#include "gp_Pnt.hxx"
//#include "BRepBuilderAPI_MakeEdge.hxx"
//#include "BRepBuilderAPI_MakeFace.hxx"
//#include "BRepBuilderAPI_MakeWire.hxx"
//#include "TopoDS_Face.hxx"
//#include "TopoDS_Wire.hxx"
//#include <BRepAlgoAPI_Cut.hxx>

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
	Box_SectionVisual::Box_SectionVisual(QString name, int id)
		: MechSectionShapeData::mSectionShapeBase(Sec_Box,name,id)
	{
	

		_para.push_back(DIM2);
		_para.push_back(DIM1);
		_para.push_back(DIM3);
		_para.push_back(DIM4);
	}

	Box_SectionVisual::~Box_SectionVisual()
	{
	}
	Box_SectionVisual* Box_SectionVisual::clone() const
	{
		Box_SectionVisual* newSec = new Box_SectionVisual(_sectionShapeName, _id);

		newSec->a = a;
		newSec->b = b;
		newSec->DIM1 = DIM1;
		newSec->DIM2 = DIM2;
		newSec->DIM3 = DIM3;
		newSec->DIM4 = DIM4;

		newSec->t2 = t2;
		newSec->t1 = t1;
		newSec->_X = _X;
		newSec->_Y = _Y;
		newSec->_Z = _Z;


		newSec->_shearMarker = _shearMarker;
		newSec->_a14LenDim = _a14LenDim;
		newSec->_a12LenDim = _a12LenDim;
		newSec->_E37LenDim = _E37LenDim;
		newSec->_E26LenDim = _E26LenDim;
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
	void Box_SectionVisual::CreateBoxSec()
	{
		try
		{
			//OA gp_Pnt Point1(_X, _Y, _Z), Point2(_X, -_Y, _Z), Point3(-_X, -_Y, _Z), Point4(-_X, _Y, _Z);
			//OA gp_Pnt Point5(_X + t2, _Y - t1, _Z); gp_Pnt Point6(_X + t2, -_Y + t1, _Z);
			//OA gp_Pnt Point7(-_X - t2, -_Y + t1, _Z); gp_Pnt Point8(-_X - t2, _Y - t1, _Z);
			//OA gp_Pnt Point14(0, _Y + 0.5, _Z); gp_Pnt Point12(_X - 0.5, 0, _Z); gp_Pnt PointE15(_X + t1 + 0.7, _Y - t1 - 0.5, _Z);
			//OA gp_Pnt PointE26(_X + t1 + 0.5, -_Y + t1 / 2, _Z); gp_Pnt PointE48(-_X - t1 - 0.5, _Y - t1 / 2, _Z); gp_Pnt PointE37(-_X + t2 + 0.5, -_Y + t1, _Z);
			//OA TopoDS_Edge Edge1 = BRepBuilderAPI_MakeEdge(Point1, Point2);
			//OA TopoDS_Edge Edge2 = BRepBuilderAPI_MakeEdge(Point2, Point3);
			//OA TopoDS_Edge Edge3 = BRepBuilderAPI_MakeEdge(Point3, Point4);
			//OA TopoDS_Edge Edge4 = BRepBuilderAPI_MakeEdge(Point4, Point1);
			//OA TopoDS_Edge Edge5 = BRepBuilderAPI_MakeEdge(Point5, Point6);
			//OA TopoDS_Edge Edge6 = BRepBuilderAPI_MakeEdge(Point6, Point7);
			//OA TopoDS_Edge Edge7 = BRepBuilderAPI_MakeEdge(Point7, Point8);
			//OA TopoDS_Edge Edge8 = BRepBuilderAPI_MakeEdge(Point8, Point5);
			//OA auto BoxSecWire1 = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//OA TopoDS_Shape BoxSecFace1 = BRepBuilderAPI_MakeFace(BoxSecWire1);
			//OA auto BoxSecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6, Edge7, Edge8);
			//OA TopoDS_Shape BoxSecFace2 = BRepBuilderAPI_MakeFace(BoxSecWire2);
			//OA TopoDS_Shape BoxSecFace = BRepAlgoAPI_Cut(BoxSecFace1, BoxSecFace2);

			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(BoxSecFace);
			//OA _aisshape = aisshape;
			//OA BaseshowBeamSec(aisshape);
			QString name = mPBeam::getInstance()->setDefaultNameForBeam(Sec_Box);
			/*emit AddSecSignal(name);
			emit showParaOnPanel(_para, name);*/
			//OA beamView::_nameAisPair.insert(name, aisshape);
			//OA beamView::_secNameAisPair.insert(name, aisshape);
			mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());
			//OA getDataFromBeam(name, BoxSecFace);


			//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();
			//OA GProp_GProps System;
			//OA BRepGProp::SurfaceProperties(BoxSecFace, System);
			//OA auto CentreOfMass = System.CentreOfMass();
			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(CentreOfMass.X(), CentreOfMass.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//_geoWindow->getAisContext()->Display(shearMarker, Standard_False);
			//OA beamView::_nameAisPair.insert(name, shearMarker);
			//OA _shearMarker = shearMarker;

			//OA Handle(AIS_LengthDimension) a14LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge4), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an14Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString DIM1Value = a14LenDim->GetValue();
			//OA TCollection_AsciiString DIM1text = "DIM1 = ";
			//OA an14Aspect->SetValueStringFormat(DIM1text + DIM1Value);
			//OA an14Aspect->MakeArrows3d(Standard_False);
			//OA an14Aspect->MakeText3d(Standard_False);
			//OA an14Aspect->TextAspect()->SetHeight(0.3);
			//OA an14Aspect->ArrowAspect()->SetLength(0.2);
			//OA an14Aspect->MakeTextShaded(Standard_True);
			//OA an14Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an14Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an14Aspect->SetArrowTailSize(0.3);
			//OA an14Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a14LenDim->SetTextPosition(Point14);
			//OA a14LenDim->SetDimensionAspect(an14Aspect);
			//_geoWindow->getAisContext()->Display(a14LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a14LenDim);
			//OA _a14LenDim = a14LenDim;

			//OA Handle(AIS_LengthDimension) a12LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge1), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an12Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString DIM2Value = a12LenDim->GetValue();
			//OA TCollection_AsciiString DIM2text = "DIM2 = ";
			//OA an12Aspect->SetValueStringFormat(DIM2text + DIM2Value);
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

			//OA Handle(AIS_LengthDimension) E37LenDim = new AIS_LengthDimension(Edge3, Edge7, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) E37Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString DIM4Value = E37LenDim->GetValue();
			//OA TCollection_AsciiString DIM4text = "DIM4 = ";
			//OA E37Aspect->SetValueStringFormat(DIM4text + DIM4Value);
			//OA E37Aspect->MakeArrows3d(Standard_False);
			//OA E37Aspect->MakeText3d(Standard_False);
			//OA E37Aspect->TextAspect()->SetHeight(0.3);
			//OA E37Aspect->ArrowAspect()->SetLength(0.2);
			//OA E37Aspect->MakeTextShaded(Standard_True);
			//OA E37Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA E37Aspect->MakeUnitsDisplayed(Standard_False);
			//OA E37Aspect->SetArrowTailSize(0.1);
			//OA E37Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Fit);
			//OA E37LenDim->SetTextPosition(PointE37);
			//OA E37LenDim->SetDimensionAspect(E37Aspect);
			//_geoWindow->getAisContext()->Display(E37LenDim, true);
			//OA beamView::_nameAisPair.insert(name, E37LenDim);
			//OA _E37LenDim = E37LenDim;

			//OA Handle(AIS_LengthDimension) E26LenDim = new AIS_LengthDimension(Edge2, Edge6, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) E26Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString DIM3Value = E26LenDim->GetValue();
			//OA TCollection_AsciiString DIM3text = "DIM3 = ";
			//OA E26Aspect->SetValueStringFormat(DIM3text + DIM3Value);
			//OA E26Aspect->MakeArrows3d(Standard_False);
			//OA E26Aspect->MakeText3d(Standard_False);
			//OA E26Aspect->TextAspect()->SetHeight(0.3);
			//OA E26Aspect->ArrowAspect()->SetLength(0.2);
			//OA E26Aspect->MakeTextShaded(Standard_True);
			//OA E26Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA E26Aspect->MakeUnitsDisplayed(Standard_False);
			//OA E26Aspect->SetArrowTailSize(0.1);
			//OA E26Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Fit);
			//OA E26LenDim->SetTextPosition(PointE26);
			//OA E26LenDim->SetDimensionAspect(E26Aspect);
			//_geoWindow->getAisContext()->Display(E26LenDim, true);
			//OA beamView::_nameAisPair.insert(name, E26LenDim);
			//OA _E26LenDim = E26LenDim;

			double a1 = a - t2;
			double b1 = b - t1;
			double c1 = a1, d1 = b1;
			double It = 4.0 * pow(a1*b1, 2) / (a1 / t1 + b1 / t2 + c1 / t1 + d1 / t2);
			_data.push_back(It);
			//OA _data.push_back(CentreOfMass.Y());
			//OA _data.push_back(CentreOfMass.X());
			double SFZ = (a - 2.0*t2)*t1 * 2 / _data.at(0);
			double SFY = (b - 2.0*t1)*t2 * 2 / _data.at(0);
			_data.push_back(SFY);
			_data.push_back(SFZ);
			//emit showDataOnPanel(_data);
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
		}
		/*gp_Pnt Point1(_X, _Y, _Z), Point2(_X, -_Y, _Z), Point3(-_X, -_Y, _Z), Point4(-_X, _Y, _Z);
		gp_Pnt Point5(_X + t2, _Y - t3, _Z); gp_Pnt Point6(_X+t2, -_Y+t1, _Z);
		gp_Pnt Point7(-_X-t4, -_Y+t1, _Z); gp_Pnt Point8(-_X-t4, _Y-t3, _Z);
		gp_Pnt Point14(0, _Y+0.5, _Z); gp_Pnt Point12(_X-0.5, 0, _Z); gp_Pnt PointE15(_X+t2 +0.7, _Y - t3-0.5, _Z);
		gp_Pnt PointE26(_X + t2 + 0.5, -_Y + t1/2, _Z); gp_Pnt PointE48(-_X - t4-0.5, _Y - t3/2, _Z); gp_Pnt PointE37(-_X - t4 - 0.7, -_Y + t1+0.5, _Z);*/
		
		
		

		//Handle(AIS_LengthDimension) E48LenDim = new AIS_LengthDimension(Edge4, Edge8, aPlane->Pln());
		//Handle(Prs3d_DimensionAspect) E48Aspect = new Prs3d_DimensionAspect();
		//TCollection_AsciiString t3Value = E48LenDim->GetValue();
		//TCollection_AsciiString t3text = "t3 = ";
		//E48Aspect->SetValueStringFormat(t3text + t3Value);
		//E48Aspect->MakeArrows3d(Standard_False);
		//E48Aspect->MakeText3d(Standard_False);
		//E48Aspect->TextAspect()->SetHeight(0.3);
		//E48Aspect->ArrowAspect()->SetLength(0.2);
		//E48Aspect->MakeTextShaded(Standard_True);
		//E48Aspect->SetCommonColor(Quantity_NOC_BLACK);
		//E48Aspect->MakeUnitsDisplayed(Standard_False);
		//E48Aspect->SetArrowTailSize(0.1);
		//E48Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Fit);
		//E48LenDim->SetTextPosition(PointE48);
		//E48LenDim->SetDimensionAspect(E48Aspect);
		//_geoWindow->getAisContext()->Display(E48LenDim, true);
		//mBeam::getInstance()->_nameAisPair.insert(name, E48LenDim);

		//Handle(AIS_LengthDimension) E37LenDim = new AIS_LengthDimension(Edge3, Edge7, aPlane->Pln());
		//Handle(Prs3d_DimensionAspect) E37Aspect = new Prs3d_DimensionAspect();
		//TCollection_AsciiString t4Value = E37LenDim->GetValue();
		//TCollection_AsciiString t4text = "t4 = ";
		//E37Aspect->SetValueStringFormat(t4text + t4Value);
		//E37Aspect->MakeArrows3d(Standard_False);
		//E37Aspect->MakeText3d(Standard_False);
		//E37Aspect->TextAspect()->SetHeight(0.3);
		//E37Aspect->ArrowAspect()->SetLength(0.2);
		//E37Aspect->MakeTextShaded(Standard_True);
		//E37Aspect->SetCommonColor(Quantity_NOC_BLACK);
		//E37Aspect->MakeUnitsDisplayed(Standard_False);
		//E37Aspect->SetArrowTailSize(0.1);
		//E37Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Fit);
		//E37LenDim->SetTextPosition(PointE37);
		//E37LenDim->SetDimensionAspect(E37Aspect);
		//_geoWindow->getAisContext()->Display(E37LenDim, true);
		//mBeam::getInstance()->_nameAisPair.insert(name, E37LenDim);

		//计算扭转常数
		
	}

	void Box_SectionVisual::ReBuildBoxSec(QList<double> para)
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


			a = DIM2, b = DIM1, t1 = DIM3, t2 = DIM4, _X = -b / 2, _Y = a / 2, _Z = 0;
			/*	gp_Pnt Point1(_X, _Y, _Z), Point2(_X, -_Y, _Z), Point3(-_X, -_Y, _Z), Point4(-_X, _Y, _Z);
				gp_Pnt Point5(_X + t2, _Y - t3, _Z); gp_Pnt Point6(_X + t2, -_Y + t1, _Z);
				gp_Pnt Point7(-_X - t4, -_Y + t1, _Z); gp_Pnt Point8(-_X - t4, _Y - t3, _Z);
				gp_Pnt Point14(0, _Y + 0.5, _Z); gp_Pnt Point12(_X - 0.5, 0, _Z); gp_Pnt PointE15(_X + t2 + 0.7, _Y - t3 - 0.5, _Z);
				gp_Pnt PointE26(_X + t2 + 0.5, -_Y + t1 / 2, _Z); gp_Pnt PointE48(-_X - t4 - 0.5, _Y - t3 / 2, _Z); gp_Pnt PointE37(-_X - t4 - 0.7, -_Y + t1 + 0.5, _Z);*/
			//OA gp_Pnt Point1(_X, _Y, _Z), Point2(_X, -_Y, _Z), Point3(-_X, -_Y, _Z), Point4(-_X, _Y, _Z);
			//OA gp_Pnt Point5(_X + t2, _Y - t1, _Z); gp_Pnt Point6(_X + t2, -_Y + t1, _Z);
			//OA gp_Pnt Point7(-_X - t2, -_Y + t1, _Z); gp_Pnt Point8(-_X - t2, _Y - t1, _Z);
			//OA gp_Pnt Point14(0, _Y + 0.5, _Z); gp_Pnt Point12(_X - 0.5, 0, _Z); gp_Pnt PointE15(_X + t1 + 0.7, _Y - t1 - 0.5, _Z);
			//OA gp_Pnt PointE26(_X + t1 + 0.5, -_Y + t1 / 2, _Z); gp_Pnt PointE48(-_X - t1 - 0.5, _Y - t1 / 2, _Z); gp_Pnt PointE37(-_X + t2 + 0.5, -_Y + t1, _Z);
			//OA TopoDS_Edge Edge1 = BRepBuilderAPI_MakeEdge(Point1, Point2);
			//OA TopoDS_Edge Edge2 = BRepBuilderAPI_MakeEdge(Point2, Point3);
			//OA TopoDS_Edge Edge3 = BRepBuilderAPI_MakeEdge(Point3, Point4);
			//OA TopoDS_Edge Edge4 = BRepBuilderAPI_MakeEdge(Point4, Point1);
			//OA TopoDS_Edge Edge5 = BRepBuilderAPI_MakeEdge(Point5, Point6);
			//OA TopoDS_Edge Edge6 = BRepBuilderAPI_MakeEdge(Point6, Point7);
			//OA TopoDS_Edge Edge7 = BRepBuilderAPI_MakeEdge(Point7, Point8);
			//OA TopoDS_Edge Edge8 = BRepBuilderAPI_MakeEdge(Point8, Point5);
			//OA auto BoxSecWire1 = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//OA TopoDS_Shape BoxSecFace1 = BRepBuilderAPI_MakeFace(BoxSecWire1);
			//OA auto BoxSecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6, Edge7, Edge8);
			//OA TopoDS_Shape BoxSecFace2 = BRepBuilderAPI_MakeFace(BoxSecWire2);
			//OA TopoDS_Shape BoxSecFace = BRepAlgoAPI_Cut(BoxSecFace1, BoxSecFace2);

			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(BoxSecFace);
			//OA _aisshape = aisshape;
			//OA BaseshowBeamSec(aisshape);
			QString name = mPBeam::getInstance()->_nameParaListPair.key(getPara());
			QList<QString> NAMElist = beamView::_nameAisPair.keys();
			if (!NAMElist.contains(name))
			{
			/*	emit AddSecSignal(name);
				emit showParaOnPanel(_para);*/
			}
			else
			{
				beamView::_nameAisPair.remove(name);
				mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());
			}

			//OA beamView::_nameAisPair.insert(name, aisshape);
			//OA beamView::_secNameAisPair.insert(name, aisshape);
			//OA getDataFromBeam(name, BoxSecFace);

			//OA GProp_GProps System;
			//OA BRepGProp::SurfaceProperties(BoxSecFace, System);
			//OA auto CentreOfMass = System.CentreOfMass();
			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(CentreOfMass.X(), CentreOfMass.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//_geoWindow->getAisContext()->Display(shearMarker, Standard_False);
			//OA beamView::_nameAisPair.insert(name, shearMarker);
			//OA _shearMarker = shearMarker;

			//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();

			//OA Handle(AIS_LengthDimension) a14LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge4), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an14Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString DIM1Value = a14LenDim->GetValue();
			//OA TCollection_AsciiString DIM1text = "DIM1 = ";
			//OA an14Aspect->SetValueStringFormat(DIM1text + DIM1Value);
			//OA an14Aspect->MakeArrows3d(Standard_False);
			//OA an14Aspect->MakeText3d(Standard_False);
			//OA an14Aspect->TextAspect()->SetHeight(0.3);
			//OA an14Aspect->ArrowAspect()->SetLength(0.2);
			//OA an14Aspect->MakeTextShaded(Standard_True);
			//OA an14Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an14Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an14Aspect->SetArrowTailSize(0.3);
			//OA an14Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a14LenDim->SetTextPosition(Point14);
			//OA a14LenDim->SetDimensionAspect(an14Aspect);
			//_geoWindow->getAisContext()->Display(a14LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a14LenDim);
			//OA _a14LenDim = a14LenDim;

			//OA Handle(AIS_LengthDimension) a12LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge1), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an12Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString DIM2Value = a12LenDim->GetValue();
			//OA TCollection_AsciiString DIM2text = "DIM2 = ";
			//OA an12Aspect->SetValueStringFormat(DIM2text + DIM2Value);
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

			//OA Handle(AIS_LengthDimension) E37LenDim = new AIS_LengthDimension(Edge3, Edge7, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) E37Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString DIM4Value = E37LenDim->GetValue();
			//OA TCollection_AsciiString DIM4text = "DIM4 = ";
			//OA E37Aspect->SetValueStringFormat(DIM4text + DIM4Value);
			//OA E37Aspect->MakeArrows3d(Standard_False);
			//OA E37Aspect->MakeText3d(Standard_False);
			//OA E37Aspect->TextAspect()->SetHeight(0.3);
			//OA E37Aspect->ArrowAspect()->SetLength(0.2);
			//OA E37Aspect->MakeTextShaded(Standard_True);
			//OA E37Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA E37Aspect->MakeUnitsDisplayed(Standard_False);
			//OA E37Aspect->SetArrowTailSize(0.1);
			//OA E37Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Fit);
			//OA E37LenDim->SetTextPosition(PointE37);
			//OA E37LenDim->SetDimensionAspect(E37Aspect);
			//_geoWindow->getAisContext()->Display(E37LenDim, true);
			//OA beamView::_nameAisPair.insert(name, E37LenDim);
			//OA _E37LenDim = E37LenDim;

			//OA Handle(AIS_LengthDimension) E26LenDim = new AIS_LengthDimension(Edge2, Edge6, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) E26Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString DIM3Value = E26LenDim->GetValue();
			//OA TCollection_AsciiString DIM3text = "DIM3 = ";
			//OA E26Aspect->SetValueStringFormat(DIM3text + DIM3Value);
			//OA E26Aspect->MakeArrows3d(Standard_False);
			//OA E26Aspect->MakeText3d(Standard_False);
			//OA E26Aspect->TextAspect()->SetHeight(0.3);
			//OA E26Aspect->ArrowAspect()->SetLength(0.2);
			//OA E26Aspect->MakeTextShaded(Standard_True);
			//OA E26Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA E26Aspect->MakeUnitsDisplayed(Standard_False);
			//OA E26Aspect->SetArrowTailSize(0.1);
			//OA E26Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Fit);
			//OA E26LenDim->SetTextPosition(PointE26);
			//OA E26LenDim->SetDimensionAspect(E26Aspect);
			//_geoWindow->getAisContext()->Display(E26LenDim, true);
			//OA beamView::_nameAisPair.insert(name, E26LenDim);
			//OA _E26LenDim = E26LenDim;
			
				//计算扭转常数
			double a1 = a - t1;
			double b1 = b - t2;
			double c1 = a1, d1 = b1;
			double It = 2.0 * pow(a1*b1, 2)*DIM3*DIM4 / (DIM1*DIM4 + DIM2 * DIM3 - DIM3 * DIM3 - DIM4 * DIM4);
			/*double It = 4.0 * pow(a1*b1, 2) / (a1 / t1 + b1 / t2 + c1 / t1 + d1 / t2);*/
			_data.push_back(It);
			//OA _data.push_back(CentreOfMass.Y());
			//OA _data.push_back(CentreOfMass.X());
			double SFY = (a - 2.0*t1)*t2 * 2 / _data.at(0);
			double SFZ = (b - 2.0*t2)*t1 * 2 / _data.at(0);
			_data.push_back(SFY);
			_data.push_back(SFZ);
			//emit showDataOnPanel(_data);
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
		}
		

	}
}

