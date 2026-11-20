#include "I1_SectionVisual.h"
#include "mPBeam.h"
//#include "beamView.h"
//#include <Prs3d_DimensionTextHorizontalPosition.hxx>
//#include <AIS_LengthDimension.hxx>
//#include <GProp_GProps.hxx>
//#include <BRepGProp.hxx>
//#include <GC_MakePlane.hxx>
#include "Sample2D_Markers.h"
//#include "GeoWidget.h"
//#include <BRepBuilderAPI_MakeEdge.hxx>
//#include <BRepBuilderAPI_MakeFace.hxx>
//#include <BRepBuilderAPI_MakeWire.hxx>
#include <mGlobalSignals.h>
//#include <TopoDS_Wire.hxx>
#include "beamView.h"
namespace MechSectionShapeData
{
	I1_SectionVisual::I1_SectionVisual(QString name, int id)
		: mSectionShapeBase(Sec_I1,name,id)
	{

		_para.push_back((b-t2)/2.0);
		_para.push_back(t2);
		_para.push_back(a);
		_para.push_back(a+2*t1);
	}

	I1_SectionVisual::~I1_SectionVisual()
	{
	}
	I1_SectionVisual* I1_SectionVisual::clone() const
	{
		I1_SectionVisual* newI1 = new I1_SectionVisual(_sectionShapeName, _id);
		newI1->DIM1 = DIM1;
		newI1->DIM2 = DIM2;
		newI1->DIM3 = DIM3;
		newI1->DIM4 = DIM4;
		newI1->a = a;
		newI1->b = b;
		newI1->t2 = t2;
		newI1->t1 = t1;
		newI1->_X1 = _X1;
		newI1->_Y1 = _Y1;
		newI1->_Z1 = _Z1;
		newI1->_X2 = _X2;
		newI1->_Y2 = _Y2;
		newI1->_Z2 = _Z2;
		newI1->_X3 = _X3;
		newI1->_Y3 = _Y3;
		newI1->_Z3 = _Z3;
		newI1->_shearMarker = _shearMarker;
		newI1->_a811LenDim = _a811LenDim;
		newI1->_a23LenDim = _a23LenDim;
		newI1->_a16LenDim = _a16LenDim;
		newI1->_a310LenDim = _a310LenDim;
		newI1->_aisshape = _aisshape;
		newI1->_para = _para;
		newI1->_data = _data;
		newI1->area = area;
		newI1->Iz = Iz;
		newI1->Iy = Iy;
		newI1->J = J;
		newI1->Iyz = Iyz;
		newI1->K1 = K1;
		newI1->K2 = K2;

		return newI1;

	}
	void I1_SectionVisual::CreateI1Sec()
	{
		try
		{
			//OA gp_Pnt Point1(_X1, _Y1, _Z1); gp_Pnt Point6(_X1, -_Y1, _Z1); gp_Pnt Point7(-_X1, -_Y1, _Z1); gp_Pnt Point12(-_X1, _Y1, _Z1);
			//OA gp_Pnt Point2(_X2, _Y2, _Z2); gp_Pnt Point5(_X2, -_Y2, _Z2); gp_Pnt Point8(-_X2, -_Y2, _Z2); gp_Pnt Point11(-_X2, _Y2, _Z2);
			//OA gp_Pnt Point3(_X3, _Y3, _Z3); gp_Pnt Point4(_X3, -_Y3, _Z3); gp_Pnt Point9(-_X3, -_Y3, _Z3); gp_Pnt Point10(-_X3, _Y3, _Z3);
			//OA gp_Pnt Point811(-_X1 + 0.5, 0, 0); gp_Pnt Point23((_X1 + _X3) / 2.0, _Y1 + 0.5, 0); gp_Pnt Point310(0, _Y3, 0); gp_Pnt Point16(_X1 - 0.5, 0, 0);
			//OA gp_Pnt Point3up(_X3, _Y1, 0);

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
			//OA TopoDS_Wire HSecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6, Edge7, Edge8).Wire();
			//OA TopoDS_Wire HSecWire3 = BRepBuilderAPI_MakeWire(Edge9, Edge10, Edge11, Edge12).Wire();
			//OA HSecWire1.Add(HSecWire2);
			//OA HSecWire1.Add(HSecWire3);
			//OA TopoDS_Shape HSecFace = BRepBuilderAPI_MakeFace(HSecWire1);
			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(HSecFace);
			//OA _aisshape = aisshape;
			//OA BaseshowBeamSec(aisshape);


			QString name = mPBeam::getInstance()->setDefaultNameForBeam(Sec_I1);
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
			//_geoWindow->getAisContext()->Display(shearMarker, Standard_False);
			//OA beamView::_nameAisPair.insert(name, shearMarker);
			//OA _shearMarker = shearMarker;

			//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();
			//OA Handle(AIS_LengthDimension) a811LenDim = new AIS_LengthDimension(Point8, Point11, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an811Aspect = new Prs3d_DimensionAspect();
			//OA an811Aspect->ArrowAspect()->SetLength(0.2);
			//OA TCollection_AsciiString aValue = a811LenDim->GetValue();
			//OA TCollection_AsciiString atext = "DIM3 = ";
			//OA an811Aspect->SetValueStringFormat(atext + aValue);
			//OA an811Aspect->MakeArrows3d(Standard_False);
			//OA an811Aspect->MakeText3d(Standard_False);
			//OA an811Aspect->TextAspect()->SetHeight(0.3);
			//OA an811Aspect->MakeTextShaded(Standard_True);
			//OA an811Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an811Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an811Aspect->SetArrowTailSize(0.3);
			//OA an811Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a811LenDim->SetTextPosition(Point811);
			//OA a811LenDim->SetDimensionAspect(an811Aspect);
			//_geoWindow->getAisContext()->Display(a811LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a811LenDim);
			//OA _a811LenDim = a811LenDim;

			//OA Handle(AIS_LengthDimension) a23LenDim = new AIS_LengthDimension(Point1, Point3up, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an23Aspect = new Prs3d_DimensionAspect();
			//OA Handle(Prs3d_ArrowAspect) Arrow23Aspect = new Prs3d_ArrowAspect();
			//OA an23Aspect->ArrowAspect()->SetLength(0.2);
			//OA TCollection_AsciiString bValue = a23LenDim->GetValue();
			//OA TCollection_AsciiString btext = "0.5*DIM1 = ";
			//OA an23Aspect->SetValueStringFormat(btext + bValue);
			//OA an23Aspect->MakeArrows3d(Standard_False);
			//OA an23Aspect->MakeText3d(Standard_False);
			//OA an23Aspect->TextAspect()->SetHeight(0.3);
			//OA an23Aspect->MakeTextShaded(Standard_True);
			//OA an23Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an23Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an23Aspect->SetArrowTailSize(0.3);
			//OA an23Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a23LenDim->SetTextPosition(Point23);
			//OA a23LenDim->SetDimensionAspect(an23Aspect);
			//_geoWindow->getAisContext()->Display(a23LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a23LenDim);
			//OA _a23LenDim = a23LenDim;


			//OA Handle(AIS_LengthDimension) a16LenDim = new AIS_LengthDimension(Point1, Point6, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an16Aspect = new Prs3d_DimensionAspect();
			//OA an16Aspect->ArrowAspect()->SetLength(0.2);
			//OA TCollection_AsciiString DIM4Value = a16LenDim->GetValue();
			//OA TCollection_AsciiString DIM4text = "DIM4 = ";
			//OA an16Aspect->SetValueStringFormat(DIM4text + DIM4Value);
			//OA an16Aspect->MakeArrows3d(Standard_False);
			//OA an16Aspect->MakeText3d(Standard_False);
			//OA an16Aspect->TextAspect()->SetHeight(0.3);
			//OA an16Aspect->MakeTextShaded(Standard_True);
			//OA an16Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an16Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an16Aspect->SetArrowTailSize(0.3);
			//OA an16Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a16LenDim->SetTextPosition(Point16);
			//OA a16LenDim->SetDimensionAspect(an16Aspect);
			//_geoWindow->getAisContext()->Display(a16LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a16LenDim);
			//OA _a16LenDim = a16LenDim;

			//OA Handle(AIS_LengthDimension) a310LenDim = new AIS_LengthDimension(Point3, Point10, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an310Aspect = new Prs3d_DimensionAspect();
			//OA an310Aspect->ArrowAspect()->SetLength(0.2);
			//OA TCollection_AsciiString t2Value = a310LenDim->GetValue();
			//OA TCollection_AsciiString t2text = "DIM2 = ";
			//OA an310Aspect->SetValueStringFormat(t2text + t2Value);
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
			//_geoWindow->getAisContext()->Display(a310LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a310LenDim);
			//OA _a310LenDim = a310LenDim;
			//求抗扭截面系数
			double ti1, bi1, Ci1 = 1.0 / 3.0;
			bi1 = a;
			ti1 = t2;
			double It1 = Ci1 * bi1*pow(ti1, 3);
			double ti2, bi2, Ci2 = 1.0 / 3.0;
			bi2 = b;
			ti2 = t1;
			double It2 = Ci2 * bi2*pow(ti2, 3);
			double It = It1 + It2 * 2;
			_data.push_back(It);
			//OA _data.push_back(CentreOfMass.Y());
			//OA _data.push_back(CentreOfMass.X());
			double SFY = a * t2 / _data.at(0);
			double SFZ = (_data.at(0) - a * t2) / _data.at(0)*5.0 / 6.0;

			_data.push_back(SFY);
			_data.push_back(SFZ);
			/*emit showDataOnPanel(_data);*/
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
		}
		

	}

	void I1_SectionVisual::ReBuildI1Sec(QList<double> para)
	{
		try
		{
			DIM1 = para.at(1) * 2.0;
			DIM2 = para.at(2);
			DIM3 = para.at(3);
			DIM4 = para.at(4);
			/*_geoWindow->getAisContext()->EraseAll(true);*/
			para.removeFirst();
			_para = para;


			b = DIM1 + DIM2, a = DIM3, t1 = (DIM4 - DIM3) / 2.0, t2 = DIM2;

			_Y1 = DIM4 / 2.0, _X1 = -((DIM1 + DIM2) / 2), _Z1 = 0;
			_Y2 = DIM3 / 2, _X2 = _X1, _Z2 = 0;
			_Y3 = _Y2, _X3 = -(DIM2 / 2), _Z3 = 0;

			//OA gp_Pnt Point1(_X1, _Y1, _Z1); gp_Pnt Point6(_X1, -_Y1, _Z1); gp_Pnt Point7(-_X1, -_Y1, _Z1); gp_Pnt Point12(-_X1, _Y1, _Z1);
			//OA gp_Pnt Point2(_X2, _Y2, _Z2); gp_Pnt Point5(_X2, -_Y2, _Z2); gp_Pnt Point8(-_X2, -_Y2, _Z2); gp_Pnt Point11(-_X2, _Y2, _Z2);
			//OA gp_Pnt Point3(_X3, _Y3, _Z3); gp_Pnt Point4(_X3, -_Y3, _Z3); gp_Pnt Point9(-_X3, -_Y3, _Z3); gp_Pnt Point10(-_X3, _Y3, _Z3);
			//OA gp_Pnt Point811(-_X1 + 0.5, 0, 0); gp_Pnt Point23((_X1 + _X3) / 2.0, _Y1 + 0.5, 0); gp_Pnt Point310(0, _Y3, 0); gp_Pnt Point16(_X1 - 0.5, 0, 0);
			//OA gp_Pnt Point3up(_X3, _Y1, 0);

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
			//OA auto H1SecWire1 = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//OA TopoDS_Wire H1SecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6, Edge7, Edge8).Wire();
			//OA TopoDS_Wire H1SecWire3 = BRepBuilderAPI_MakeWire(Edge9, Edge10, Edge11, Edge12).Wire();
			//OA H1SecWire1.Add(H1SecWire2);
			//OA H1SecWire1.Add(H1SecWire3);
			//OA TopoDS_Shape H1SecFace = BRepBuilderAPI_MakeFace(H1SecWire1);
			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(H1SecFace);
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
				mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());
				beamView::_nameAisPair.remove(name);
			}

			//OA beamView::_nameAisPair.insert(name, aisshape);
			//OA beamView::_secNameAisPair.insert(name, aisshape);
			//OA getDataFromBeam(name, H1SecFace);
			//OA GProp_GProps System;
			//OA BRepGProp::SurfaceProperties(H1SecFace, System);
			//OA auto CentreOfMass = System.CentreOfMass();
			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(CentreOfMass.X(), CentreOfMass.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//_geoWindow->getAisContext()->Display(shearMarker, Standard_False);
			//OA beamView::_nameAisPair.insert(name, shearMarker);	GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA _shearMarker = shearMarker;
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();
			//OA Handle(AIS_LengthDimension) a811LenDim = new AIS_LengthDimension(Point8, Point11, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an811Aspect = new Prs3d_DimensionAspect();
			//OA an811Aspect->ArrowAspect()->SetLength(0.2);
			//OA TCollection_AsciiString aValue = a811LenDim->GetValue();
			//OA TCollection_AsciiString atext = "DIM3 = ";
			//OA an811Aspect->SetValueStringFormat(atext + aValue);
			//OA an811Aspect->MakeArrows3d(Standard_False);
			//OA an811Aspect->MakeText3d(Standard_False);
			//OA an811Aspect->TextAspect()->SetHeight(0.3);
			//OA an811Aspect->MakeTextShaded(Standard_True);
			//OA an811Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an811Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an811Aspect->SetArrowTailSize(0.3);
			//OA an811Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a811LenDim->SetTextPosition(Point811);
			//OA a811LenDim->SetDimensionAspect(an811Aspect);
			//_geoWindow->getAisContext()->Display(a811LenDim, true);
			
			//OA beamView::_nameAisPair.insert(name, a811LenDim);
			//OA _a811LenDim = a811LenDim;
			//OA Handle(AIS_LengthDimension) a23LenDim = new AIS_LengthDimension(Point1, Point3up, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an23Aspect = new Prs3d_DimensionAspect();
			//OA Handle(Prs3d_ArrowAspect) Arrow23Aspect = new Prs3d_ArrowAspect();
			//OA an23Aspect->ArrowAspect()->SetLength(0.2);
			//OA TCollection_AsciiString bValue = a23LenDim->GetValue();
			//OA TCollection_AsciiString btext = "0.5*DIM1 = ";
			//OA an23Aspect->SetValueStringFormat(btext + bValue);
			//OA an23Aspect->MakeArrows3d(Standard_False);
			//OA an23Aspect->MakeText3d(Standard_False);
			//OA an23Aspect->TextAspect()->SetHeight(0.3);
			//OA an23Aspect->MakeTextShaded(Standard_True);
			//OA an23Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an23Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an23Aspect->SetArrowTailSize(0.3);
			//OA an23Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a23LenDim->SetTextPosition(Point23);
			//OA a23LenDim->SetDimensionAspect(an23Aspect);
			//_geoWindow->getAisContext()->Display(a23LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a23LenDim);
			//OA _a23LenDim = a23LenDim;



			//OA Handle(AIS_LengthDimension) a16LenDim = new AIS_LengthDimension(Point1, Point6, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an16Aspect = new Prs3d_DimensionAspect();
			//OA an16Aspect->ArrowAspect()->SetLength(0.2);
			//OA TCollection_AsciiString t1Value = a16LenDim->GetValue();
			//OA TCollection_AsciiString t1text = "DIM4 = ";
			//OA an16Aspect->SetValueStringFormat(t1text + t1Value);
			//OA an16Aspect->MakeArrows3d(Standard_False);
			//OA an16Aspect->MakeText3d(Standard_False);
			//OA an16Aspect->TextAspect()->SetHeight(0.3);
			//OA an16Aspect->MakeTextShaded(Standard_True);
			//OA an16Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an16Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an16Aspect->SetArrowTailSize(0.3);
			//OA an16Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a16LenDim->SetTextPosition(Point16);
			//OA a16LenDim->SetDimensionAspect(an16Aspect);
			//_geoWindow->getAisContext()->Display(a16LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a16LenDim);
			//OA _a16LenDim = a16LenDim;

			//OA Handle(AIS_LengthDimension) a310LenDim = new AIS_LengthDimension(Point3, Point10, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an310Aspect = new Prs3d_DimensionAspect();
			//OA an310Aspect->ArrowAspect()->SetLength(0.2);
			//OA TCollection_AsciiString t4Value = a310LenDim->GetValue();
			//OA TCollection_AsciiString t4text = "DIM2 = ";
			//OA an310Aspect->SetValueStringFormat(t4text + t4Value);
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
			//_geoWindow->getAisContext()->Display(a310LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a310LenDim);
			//OA _a310LenDim = a310LenDim;
			//求抗扭截面系数
			double ti1, bi1, Ci1 = 1.0 / 3.0;
			bi1 = a;
			ti1 = t2;
			double It1 = Ci1 * bi1*pow(ti1, 3);
			double ti2, bi2, Ci2 = 1.0 / 3.0;
			bi2 = b;
			ti2 = t1;
			double It2 = Ci2 * bi2*pow(ti2, 3);
			double It = It1 + It2 * 2;
			_data.push_back(It);
			//OA _data.push_back(CentreOfMass.Y());
			//OA _data.push_back(CentreOfMass.X());
			double SFY = a * t2 / _data.at(0);
			double SFZ = (_data.at(0) - a * t2) / _data.at(0)*5.0 / 6.0;

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
