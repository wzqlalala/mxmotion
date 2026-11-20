#include "H_SectionVisual.h"
//#include "gp_Pnt.hxx"
//#include "BRepBuilderAPI_MakeEdge.hxx"
//#include "BRepBuilderAPI_MakeFace.hxx"
//#include "BRepBuilderAPI_MakeWire.hxx"
//#include "TopoDS_Face.hxx"
//#include "TopoDS_Wire.hxx"

//#include <Prs3d_Drawer.hxx>
//#include <AIS_InteractiveContext.hxx>

//#include "GeoWidget.h"
//#include "GC_MakePlane.hxx"
#include "mPBeam.h"
//#include <GProp_GProps.hxx>
#include <mGlobalSignals.h>

//#include "BRepGProp_Face.hxx"
//#include "BRepGProp.hxx"
#include "Sample2D_Markers.h"
#include "beamView.h"

namespace MechSectionShapeData
{
	H_SectionVisual::H_SectionVisual(QString name,int id)
		: mSectionShapeBase(Sec_H,name,id)
	{
		_para.push_back(DIM1);
		_para.push_back(0.5*DIM2);
		_para.push_back(DIM3);
		_para.push_back(DIM4);
		
	}

	H_SectionVisual::~H_SectionVisual()
	{
	}

	H_SectionVisual* H_SectionVisual::clone() const
	{
		H_SectionVisual* newH = new H_SectionVisual(_sectionShapeName, _id);
		newH->DIM1 = DIM1;
		newH->DIM2 = DIM2;
		newH->DIM3 = DIM3;
		newH->DIM4 = DIM4;
		newH->a = a;
		newH->b = b;
		newH->t2 = t2;
		newH->t1 = t1;
		newH->_X1 = _X1;
		newH->_Y1 = _Y1;
		newH->_Z1 = _Z1;
		newH->_X2 = _X2;
		newH->_Y2 = _Y2;
		newH->_Z2 = _Z2;
		newH->_X3 = _X3;
		newH->_Y3 = _Y3;
		newH->_Z3 = _Z3;
		newH->_shearMarker = _shearMarker;
		newH->_a25LenDim = _a25LenDim;
		newH->_a67LenDim = _a67LenDim;
		newH->_a1112LenDim = _a1112LenDim;
		newH->_a310LenDim = _a310LenDim;
		newH->_aisshape = _aisshape;
		newH->_para = _para;
		newH->_data = _data;
		newH->area = area;
		newH->Iz = Iz;
		newH->Iy = Iy;
		newH->J = J;
		newH->Iyz = Iyz;
		newH->K1 = K1;
		newH->K2 = K2;

		return newH;
	}

	void H_SectionVisual::CreateHSec()
	{
		try
		{
			//OA gp_Pnt Point1(_X1, _Y1, _Z1); gp_Pnt Point6(-_X1, _Y1, _Z1); gp_Pnt Point7(-_X1, -_Y1, _Z1); gp_Pnt Point12(_X1, -_Y1, _Z1);
			//OA gp_Pnt Point2(_X2, _Y2, _Z2); gp_Pnt Point5(-_X2, _Y2, _Z2); gp_Pnt Point8(-_X2, -_Y2, _Z2); gp_Pnt Point11(_X2, -_Y2, _Z2);
			//OA gp_Pnt Point3(_X3, _Y3, _Z3); gp_Pnt Point4(-_X3, _Y3, _Z3); gp_Pnt Point9(-_X3, -_Y3, _Z3); gp_Pnt Point10(_X3, -_Y3, _Z3);
			//OA gp_Pnt Point1112Ref(_X1 + 0.25*DIM2, -_Y1 - 0.5, 0); gp_Pnt Point25(0, _Y1 + 0.5, 0); gp_Pnt Point310(_X2, 0, 0); gp_Pnt Point67(-_X1 + 0.5, 0, 0);

			//OA TopoDS_Edge Edge1 = BRepBuilderAPI_MakeEdge(Point1, Point2);
			//OA TopoDS_Edge Edge2 = BRepBuilderAPI_MakeEdge(Point2, Point3);
			//OA TopoDS_Edge Edge3 = BRepBuilderAPI_MakeEdge(Point3, Point4);
			//OA TopoDS_Edge Edge4 = BRepBuilderAPI_MakeEdge(Point4, Point5);
			//OA TopoDS_Edge Edge5 = BRepBuilderAPI_MakeEdge(Point5, Point6);
			//OA TopoDS_Edge Edge6 = BRepBuilderAPI_MakeEdge(Point6, Point7);
			//OA TopoDS_Edge Edge7 = BRepBuilderAPI_MakeEdge(Point7, Point8);
			//OA TopoDS_Edge Edge8 = BRepBuilderAPI_MakeEdge(Point8, Point9);
			//OA TopoDS_Edge Edge9 = BRepBuilderAPI_MakeEdge(Point9, Point10);
			//OA TopoDS_Edge Edge10 = BRepBuilderAPI_MakeEdge(Point10, Point11);
			//OA TopoDS_Edge Edge11 = BRepBuilderAPI_MakeEdge(Point11, Point12);
			//OA TopoDS_Edge Edge12 = BRepBuilderAPI_MakeEdge(Point12, Point1);
			//OA auto HSecWire1 = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//OA TopoDS_Wire HSecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6, Edge7, Edge8);
			//OA TopoDS_Wire HSecWire3 = BRepBuilderAPI_MakeWire(Edge9, Edge10, Edge11, Edge12);
			//OA HSecWire1.Add(HSecWire2);
			//OA HSecWire1.Add(HSecWire3);
			//OA TopoDS_Shape HSecFace = BRepBuilderAPI_MakeFace(HSecWire1);
			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(HSecFace);
			//OA _aisshape = aisshape;
			//OA BaseshowBeamSec(aisshape);


			QString name = mPBeam::getInstance()->setDefaultNameForBeam(Sec_H);
			/*emit AddSecSignal(name);
			emit showParaOnPanel(_para, name);*/

			//OA beamView::_nameAisPair.insert(name, aisshape);
			//OA beamView::_secNameAisPair.insert(name, aisshape);
			mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());
			//OA getDataFromBeam(name, HSecFace);
			//OA GProp_GProps System;
			//OA BRepGProp::SurfaceProperties(HSecFace, System);
			//OA auto CentreOfMass = System.CentreOfMass();
			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(CentreOfMass.X(), CentreOfMass.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//OA _shearMarker = shearMarker;
			
			//OA beamView::_nameAisPair.insert(name, shearMarker);


			//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();
			//OA Handle(AIS_LengthDimension) a25LenDim = new AIS_LengthDimension(Point2, Point5, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an25Aspect = new Prs3d_DimensionAspect();
			//OA an25Aspect->ArrowAspect()->SetLength(0.2);
			//OA TCollection_AsciiString DIM1Value = a25LenDim->GetValue();
			//OA TCollection_AsciiString DIM1text = "DIM1 = ";
			//OA an25Aspect->SetValueStringFormat(DIM1text + DIM1Value);
			//OA an25Aspect->MakeArrows3d(Standard_False);
			//OA an25Aspect->MakeText3d(Standard_False);
			//OA an25Aspect->TextAspect()->SetHeight(0.3);
			//OA an25Aspect->MakeTextShaded(Standard_True);
			//OA an25Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an25Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an25Aspect->SetArrowTailSize(0.3);
			//OA an25Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a25LenDim->SetTextPosition(Point25);
			//OA a25LenDim->SetDimensionAspect(an25Aspect);
			//OA _a25LenDim = a25LenDim;
		
			//OA beamView::_nameAisPair.insert(name, a25LenDim);

			//OA Handle(AIS_LengthDimension) a67LenDim = new AIS_LengthDimension(Point6, Point7, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an67Aspect = new Prs3d_DimensionAspect();
			//OA Handle(Prs3d_ArrowAspect) Arrow67Aspect = new Prs3d_ArrowAspect();
			//OA an67Aspect->ArrowAspect()->SetLength(0.2);
			//OA TCollection_AsciiString DIM3Value = a67LenDim->GetValue();
			//OA TCollection_AsciiString DIM3text = "DIM3 = ";
			//OA an67Aspect->SetValueStringFormat(DIM3text + DIM3Value);
			//OA an67Aspect->MakeArrows3d(Standard_False);
			//OA an67Aspect->MakeText3d(Standard_False);
			//OA an67Aspect->TextAspect()->SetHeight(0.3);
			//OA an67Aspect->MakeTextShaded(Standard_True);
			//OA an67Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an67Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an67Aspect->SetArrowTailSize(0.3);
			//OA an67Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a67LenDim->SetTextPosition(Point67);
			//OA a67LenDim->SetDimensionAspect(an67Aspect);
			//OA _a67LenDim = a67LenDim;
		
			//OA beamView::_nameAisPair.insert(name, a67LenDim);

			//OA Handle(AIS_LengthDimension) a1112LenDim = new AIS_LengthDimension(Point11, Point12, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an1112Aspect = new Prs3d_DimensionAspect();
			//OA an1112Aspect->ArrowAspect()->SetLength(0.2);
			//OA TCollection_AsciiString DIM2Value = a1112LenDim->GetValue();
			//OA TCollection_AsciiString DIM2text = "0.5*DIM2 = ";
			//OA an1112Aspect->SetValueStringFormat(DIM2text + DIM2Value);
			//OA an1112Aspect->MakeArrows3d(Standard_False);
			//OA an1112Aspect->MakeText3d(Standard_False);
			//OA an1112Aspect->TextAspect()->SetHeight(0.3);
			//OA an1112Aspect->MakeTextShaded(Standard_True);
			//OA an1112Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an1112Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an1112Aspect->SetArrowTailSize(0.3);
			//OA an1112Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a1112LenDim->SetTextPosition(Point1112Ref);
			//OA a1112LenDim->SetDimensionAspect(an1112Aspect);
			//OA _a1112LenDim = a1112LenDim;

			//OA beamView::_nameAisPair.insert(name, a1112LenDim);

			//OA Handle(AIS_LengthDimension) a310LenDim = new AIS_LengthDimension(Point3, Point10, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an310Aspect = new Prs3d_DimensionAspect();
			//OA an310Aspect->ArrowAspect()->SetLength(0.2);
			//OA TCollection_AsciiString DIM4Value = a310LenDim->GetValue();
			//OA TCollection_AsciiString DIM4text = "DIM4 = ";
			//OA an310Aspect->SetValueStringFormat(DIM4text + DIM4Value);
			//OA an310Aspect->MakeArrows3d(Standard_False);
			//OA an310Aspect->MakeText3d(Standard_False);
			//OA an310Aspect->TextAspect()->SetHeight(0.3);
			//OA an310Aspect->MakeTextShaded(Standard_True);
			//OA an310Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an310Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an310Aspect->SetArrowTailSize(0.3);
			//OA an310Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a310LenDim->SetTextPosition(Point310);
			//OA a310LenDim->SetDimensionAspect(an310Aspect);
			//OA _a310LenDim = a310LenDim;
	
			//OA beamView::_nameAisPair.insert(name, a310LenDim);
			//求抗扭截面系数
			double ti1, bi1, Ci1 = 1.0 / 3.0;
			bi1 = a;
			ti1 = t1;
			double It1 = Ci1 * bi1*pow(ti1, 3);
			double ti2, bi2, Ci2 = 1.0 / 3.0;
			bi2 = b;
			ti2 = t2;
			double It2 = Ci2 * bi2*pow(ti2, 3);
			double It = It1 + It2 * 2;
			_data.push_back(It);
			//OA _data.push_back(CentreOfMass.Y());
			//OA _data.push_back(CentreOfMass.X());
			double SFZ = a * t1 / _data.at(0);
			double SFY = (_data.at(0) - a * t1) / _data.at(0)*5.0 / 6.0;

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

	void H_SectionVisual::ReBuildHSec(QList<double> para)
	{
		try
		{
			DIM1 = para.at(1);
			DIM2 = para.at(2) * 2;
			DIM3 = para.at(3);
			DIM4 = para.at(4);
			para.removeFirst();
			//_geoWindow->getAisContext()->EraseAll(true);
			_para = para;

			a = DIM1, b = DIM3, t2 = 0.5*DIM2, t1 = DIM4;
			_X1 = -(a / 2 + t2), _Y1 = (b / 2.0), _Z1 = 0;
			_X2 = -a / 2, _Y2 = (b / 2.0), _Z2 = 0;
			_X3 = _X2, _Y3 = t1 / 2, _Z3 = 0;

			//OA gp_Pnt Point1(_X1, _Y1, _Z1); gp_Pnt Point6(-_X1, _Y1, _Z1); gp_Pnt Point7(-_X1, -_Y1, _Z1); gp_Pnt Point12(_X1, -_Y1, _Z1);
			//OA gp_Pnt Point2(_X2, _Y2, _Z2); gp_Pnt Point5(-_X2, _Y2, _Z2); gp_Pnt Point8(-_X2, -_Y2, _Z2); gp_Pnt Point11(_X2, -_Y2, _Z2);
			//OA gp_Pnt Point3(_X3, _Y3, _Z3); gp_Pnt Point4(-_X3, _Y3, _Z3); gp_Pnt Point9(-_X3, -_Y3, _Z3); gp_Pnt Point10(_X3, -_Y3, _Z3);
			//OA gp_Pnt Point1112Ref(_X1 + 0.25*DIM2, -_Y1 - 0.5, 0); gp_Pnt Point25(0, _Y1 + 0.5, 0); gp_Pnt Point310(_X2, 0, 0); gp_Pnt Point67(-_X1 + 0.5, 0, 0);


			//OA TopoDS_Edge Edge1 = BRepBuilderAPI_MakeEdge(Point1, Point2);
			//OA TopoDS_Edge Edge2 = BRepBuilderAPI_MakeEdge(Point2, Point3);
			//OA TopoDS_Edge Edge3 = BRepBuilderAPI_MakeEdge(Point3, Point4);
			//OA TopoDS_Edge Edge4 = BRepBuilderAPI_MakeEdge(Point4, Point5);
			//OA TopoDS_Edge Edge5 = BRepBuilderAPI_MakeEdge(Point5, Point6);
			//OA TopoDS_Edge Edge6 = BRepBuilderAPI_MakeEdge(Point6, Point7);
			//OA TopoDS_Edge Edge7 = BRepBuilderAPI_MakeEdge(Point7, Point8);
			//OA TopoDS_Edge Edge8 = BRepBuilderAPI_MakeEdge(Point8, Point9);
			//OA TopoDS_Edge Edge9 = BRepBuilderAPI_MakeEdge(Point9, Point10);
			//OA TopoDS_Edge Edge10 = BRepBuilderAPI_MakeEdge(Point10, Point11);
			//OA TopoDS_Edge Edge11 = BRepBuilderAPI_MakeEdge(Point11, Point12);
			//OA TopoDS_Edge Edge12 = BRepBuilderAPI_MakeEdge(Point12, Point1);
			//OA auto HSecWire1 = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//OA TopoDS_Wire HSecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6, Edge7, Edge8);
			//OA TopoDS_Wire HSecWire3 = BRepBuilderAPI_MakeWire(Edge9, Edge10, Edge11, Edge12);
			//OA HSecWire1.Add(HSecWire2);
			//OA HSecWire1.Add(HSecWire3);
			//OA TopoDS_Shape HSecFace = BRepBuilderAPI_MakeFace(HSecWire1);
			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(HSecFace);
			//OA _aisshape = aisshape;
			//OA BaseshowBeamSec(aisshape);
			QString name = mPBeam::getInstance()->_nameParaListPair.key(getPara());;
			QList<QString> NAMElist = beamView::_nameAisPair.keys();
			if (!NAMElist.contains(name))
			{
				//emit AddSecSignal(name);
				//emit showParaOnPanel(_para);
			}
			else
			{
				mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());
				beamView::_nameAisPair.remove(name);
			}

			//OA beamView::_nameAisPair.insert(name, aisshape);
			//OA beamView::_secNameAisPair.insert(name, aisshape);

			//OA getDataFromBeam(name, HSecFace);

			//OA GProp_GProps System;
			//OA BRepGProp::SurfaceProperties(HSecFace, System);
			//OA auto CentreOfMass = System.CentreOfMass();
			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(CentreOfMass.X(), CentreOfMass.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//OA _shearMarker = shearMarker;
			//_geoWindow->getAisContext()->Display(shearMarker, Standard_False);
			//OA beamView::_nameAisPair.insert(name, shearMarker);


			//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();
			//OA Handle(AIS_LengthDimension) a25LenDim = new AIS_LengthDimension(Point2, Point5, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an25Aspect = new Prs3d_DimensionAspect();
			//OA an25Aspect->ArrowAspect()->SetLength(0.2);
			//OA TCollection_AsciiString DIM1Value = a25LenDim->GetValue();
			//OA TCollection_AsciiString DIM1text = "DIM1 = ";
			//OA an25Aspect->SetValueStringFormat(DIM1text + DIM1Value);
			//OA an25Aspect->MakeArrows3d(Standard_False);
			//OA an25Aspect->MakeText3d(Standard_False);
			//OA an25Aspect->TextAspect()->SetHeight(0.3);
			//OA an25Aspect->MakeTextShaded(Standard_True);
			//OA an25Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an25Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an25Aspect->SetArrowTailSize(0.3);
			//OA an25Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a25LenDim->SetTextPosition(Point25);
			//OA a25LenDim->SetDimensionAspect(an25Aspect);
			//OA _a25LenDim = a25LenDim;
			//_geoWindow->getAisContext()->Display(a25LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a25LenDim);

			//OA Handle(AIS_LengthDimension) a67LenDim = new AIS_LengthDimension(Point6, Point7, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an67Aspect = new Prs3d_DimensionAspect();
			//OA Handle(Prs3d_ArrowAspect) Arrow67Aspect = new Prs3d_ArrowAspect();
			//OA an67Aspect->ArrowAspect()->SetLength(0.2);
			//OA TCollection_AsciiString DIM3Value = a67LenDim->GetValue();
			//OA TCollection_AsciiString DIM3text = "DIM3 = ";
			//OA an67Aspect->SetValueStringFormat(DIM3text + DIM3Value);
			//OA an67Aspect->MakeArrows3d(Standard_False);
			//OA an67Aspect->MakeText3d(Standard_False);
			//OA an67Aspect->TextAspect()->SetHeight(0.3);
			//OA an67Aspect->MakeTextShaded(Standard_True);
			//OA an67Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an67Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an67Aspect->SetArrowTailSize(0.3);
			//OA an67Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a67LenDim->SetTextPosition(Point67);
			//OA a67LenDim->SetDimensionAspect(an67Aspect);
			//OA _a67LenDim = a67LenDim;
			//_geoWindow->getAisContext()->Display(a67LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a67LenDim);

			//OA Handle(AIS_LengthDimension) a1112LenDim = new AIS_LengthDimension(Point11, Point12, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an1112Aspect = new Prs3d_DimensionAspect();
			//OA an1112Aspect->ArrowAspect()->SetLength(0.2);
			//OA TCollection_AsciiString DIM2Value = a1112LenDim->GetValue();
			//OA TCollection_AsciiString DIM2text = "0.5*DIM2 = ";
			//OA an1112Aspect->SetValueStringFormat(DIM2text + DIM2Value);
			//OA an1112Aspect->MakeArrows3d(Standard_False);
			//OA an1112Aspect->MakeText3d(Standard_False);
			//OA an1112Aspect->TextAspect()->SetHeight(0.3);
			//OA an1112Aspect->MakeTextShaded(Standard_True);
			//OA an1112Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an1112Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an1112Aspect->SetArrowTailSize(0.3);
			//OA an1112Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a1112LenDim->SetTextPosition(Point1112Ref);
			//OA a1112LenDim->SetDimensionAspect(an1112Aspect);
			//OA _a1112LenDim = a1112LenDim;
			//_geoWindow->getAisContext()->Display(a1112LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a1112LenDim);

			//OA Handle(AIS_LengthDimension) a310LenDim = new AIS_LengthDimension(Point3, Point10, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an310Aspect = new Prs3d_DimensionAspect();
			//OA an310Aspect->ArrowAspect()->SetLength(0.2);
			//OA TCollection_AsciiString DIM4Value = a310LenDim->GetValue();
			//OA TCollection_AsciiString DIM4text = "DIM4 = ";
			//OA an310Aspect->SetValueStringFormat(DIM4text + DIM4Value);
			//OA an310Aspect->MakeArrows3d(Standard_False);
			//OA an310Aspect->MakeText3d(Standard_False);
			//OA an310Aspect->TextAspect()->SetHeight(0.3);
			//OA an310Aspect->MakeTextShaded(Standard_True);
			//OA an310Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an310Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an310Aspect->SetArrowTailSize(0.3);
			//OA an310Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a310LenDim->SetTextPosition(Point310);
			//OA a310LenDim->SetDimensionAspect(an310Aspect);
			//OA _a310LenDim = a310LenDim;
			//_geoWindow->getAisContext()->Display(a310LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a310LenDim);
			//求抗扭截面系数
			double ti1, bi1, Ci1 = 1.0 / 3.0;
			bi1 = a;
			ti1 = t1;
			double It1 = Ci1 * bi1*pow(ti1, 3);
			double ti2, bi2, Ci2 = 1.0 / 3.0;
			bi2 = b;
			ti2 = t2;
			double It2 = Ci2 * bi2*pow(ti2, 3);
			double It = It1 + It2 * 2;
			_data.push_back(It);
			//OA _data.push_back(CentreOfMass.Y());
			//OA _data.push_back(CentreOfMass.X());
			double SFZ = a * t1 / _data.at(0);
			double SFY = (_data.at(0) - a * t1) / _data.at(0)*5.0 / 6.0;
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

