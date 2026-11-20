#include "T_SectionVisual.h"
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
	T_SectionVisual::T_SectionVisual(QString name, int id)
		: MechSectionShapeData::mSectionShapeBase(Sec_T,name,id)
	{
		_para.push_back(a);
		_para.push_back(b);
		_para.push_back(t2);
		_para.push_back(t1);
	}

	T_SectionVisual::~T_SectionVisual()
	{
	}
	T_SectionVisual* T_SectionVisual::clone() const
	{


		T_SectionVisual* newSec = new T_SectionVisual(_sectionShapeName, _id);

		newSec->a = a;
		newSec->b = b;
		newSec->c = c;
		newSec->t2 = t2;
		newSec->t1 = t1;
		newSec->_X1 = _X1;
		newSec->_X2 = _X2;
		newSec->_X3 = _X3;
		newSec->_Y1 = _Y1;
		newSec->_Y2 = _Y2;
		newSec->_Y3 = _Y3;
		newSec->_Z1 = _Z1;
		newSec->_Z2 = _Z2;
		newSec->_Z3 = _Z3;

		newSec->_shearMarker = _shearMarker;
		newSec->_a18LenDim = _a18LenDim;
		newSec->_a12LenDim = _a12LenDim;
		newSec->_a36LenDim = _a36LenDim;
		newSec->_E84LenDim = _E84LenDim;
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
	void T_SectionVisual::CreateTSec()
	{
		try
		{
			//OA gp_Pnt Point1(_X1, _Y1, _Z1); gp_Pnt Point8(-_X1, _Y1, _Z1);
			//OA gp_Pnt Point2(_X2, _Y2, _Z2); gp_Pnt Point7(-_X2, _Y2, _Z2);
			//OA gp_Pnt Point3(_X3, _Y3, _Z3); gp_Pnt Point4(_X3, -_Y3 - t2, _Z3); gp_Pnt Point5(-_X3, -_Y3 - t2, _Z3); gp_Pnt Point6(-_X3, _Y3, _Z3);
			//OA gp_Pnt Point18(0, _Y1 + 0.5, _Z3); gp_Pnt Point12(_X1 - 0.5, (_Y1 + _Y2) / 2, _Z3); gp_Pnt Point36(0, _Y3 - 0.5, _Z3); gp_Pnt PointE84(-_X1, 0, _Z3);
			//OA gp_Pnt shearCenter(0, (_Y1 + _Y2) / 2, _Z3);
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
			QString name = mPBeam::getInstance()->setDefaultNameForBeam(Sec_T);

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


			//OA Handle(AIS_LengthDimension) a18LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge8), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an18Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString aValue = a18LenDim->GetValue();
			//OA TCollection_AsciiString atext = "DIM1 = ";
			//OA an18Aspect->SetValueStringFormat(atext + aValue);
			//OA an18Aspect->MakeArrows3d(Standard_False);
			//OA an18Aspect->MakeText3d(Standard_False);
			//OA an18Aspect->TextAspect()->SetHeight(0.3);
			//OA an18Aspect->ArrowAspect()->SetLength(0.2);
			//OA an18Aspect->MakeTextShaded(Standard_True);
			//OA an18Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an18Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an18Aspect->SetArrowTailSize(0.3);
			//OA an18Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a18LenDim->SetTextPosition(Point18);
			//OA a18LenDim->SetDimensionAspect(an18Aspect);
			//_geoWindow->getAisContext()->Display(a18LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a18LenDim);
			//OA _a18LenDim = a18LenDim;

			//OA Handle(AIS_LengthDimension) a12LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge1), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an12Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t2Value = a12LenDim->GetValue();
			//OA TCollection_AsciiString t2text = "DIM3 = ";
			//OA an12Aspect->SetValueStringFormat(t2text + t2Value);
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

			//OA Handle(AIS_LengthDimension) a36LenDim = new AIS_LengthDimension(Point3, Point6, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an36Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t1Value = a36LenDim->GetValue();
			//OA TCollection_AsciiString t1text = "DIM4 = ";
			//OA an36Aspect->SetValueStringFormat(t1text + t1Value);
			//OA an36Aspect->MakeArrows3d(Standard_False);
			//OA an36Aspect->MakeText3d(Standard_False);
			//OA an36Aspect->TextAspect()->SetHeight(0.3);
			//OA an36Aspect->ArrowAspect()->SetLength(0.2);
			//OA an36Aspect->MakeTextShaded(Standard_True);
			//OA an36Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an36Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an36Aspect->SetArrowTailSize(0.3);
			//OA an36Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a36LenDim->SetTextPosition(Point36);
			//OA a36LenDim->SetDimensionAspect(an36Aspect);
			//_geoWindow->getAisContext()->Display(a36LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a36LenDim);
			//OA _a36LenDim = a36LenDim;

			//OA Handle(AIS_LengthDimension) E84LenDim = new AIS_LengthDimension(Edge4, Edge8, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an84Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValue = E84LenDim->GetValue();
			//OA TCollection_AsciiString btext = "DIM2 = ";
			//OA an84Aspect->SetValueStringFormat(btext + bValue);
			//OA an84Aspect->MakeArrows3d(Standard_False);
			//OA an84Aspect->MakeText3d(Standard_False);
			//OA an84Aspect->TextAspect()->SetHeight(0.3);
			//OA an84Aspect->ArrowAspect()->SetLength(0.2);
			//OA an84Aspect->MakeTextShaded(Standard_True);
			//OA an84Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an84Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an84Aspect->SetArrowTailSize(0.3);
			//OA an84Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA E84LenDim->SetTextPosition(PointE84);
			//OA E84LenDim->SetDimensionAspect(an84Aspect);
			//_geoWindow->getAisContext()->Display(E84LenDim, true);
			//OA beamView::_nameAisPair.insert(name, E84LenDim);
			//OA _E84LenDim = E84LenDim;

			//求抗扭截面系数
			double ti1, bi1, Ci1 = 1.0 / 3.0;

			bi1 = a;
			ti1 = t2;

			double It1 = Ci1 * bi1*pow(ti1, 3);

			double ti2, bi2, Ci2 = 1.0 / 3.0;

			bi2 = b - t2;
			ti2 = t1;
			double It2 = Ci2 * bi2*pow(ti2, 3);
			double It = It1 + It2 + 1.0 / 3.0*t2*pow(t1, 3) / 2;
			_data.push_back(It);
			//OA _data.push_back(shearCenter.Y());
			//OA _data.push_back(shearCenter.X());
			double SFZ = (a - t1)*t2 / _data.at(0);
			double SFY = (b - t2)*t1 / _data.at(0);
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

	void T_SectionVisual::ReBuildTSec(QList<double> para)
	{
		try
		{
			a = para.at(1);
			b = para.at(2);
			t1 = para.at(4);
			t2 = para.at(3);
			//_geoWindow->getAisContext()->EraseAll(true);
			para.removeFirst();
			_para = para;

			_Y1 = b / 2, _X1 = -a / 2, _Z1 = 0;
			_Y2 = _Y1 - t2, _X2 = _X1, _Z2 = 0;
			_Y3 = _Y2, _X3 = -t1 / 2, _Z3 = 0;
			//OA gp_Pnt Point1(_X1, _Y1, _Z1); gp_Pnt Point8(-_X1, _Y1, _Z1);
			//OA gp_Pnt Point2(_X2, _Y2, _Z2); gp_Pnt Point7(-_X2, _Y2, _Z2);
			//OA gp_Pnt Point3(_X3, _Y3, _Z3); gp_Pnt Point4(_X3, -_Y3 - t2, _Z3); gp_Pnt Point5(-_X3, -_Y3 - t2, _Z3); gp_Pnt Point6(-_X3, _Y3, _Z3);
			//OA gp_Pnt Point18(0, _Y1 + 0.5, _Z3); gp_Pnt Point12(_X1 - 0.5, (_Y1 + _Y2) / 2, _Z3); gp_Pnt Point36(0, _Y3 - 0.5, _Z3); gp_Pnt PointE84(-_X1, 0, _Z3);
			//OA gp_Pnt shearCenter(0, (_Y1 + _Y2) / 2, _Z3);
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




			//OA Handle(AIS_LengthDimension) a18LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge8), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an18Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString aValue = a18LenDim->GetValue();
			//OA TCollection_AsciiString atext = "DIM1 = ";
			//OA an18Aspect->SetValueStringFormat(atext + aValue);
			//OA an18Aspect->MakeArrows3d(Standard_False);
			//OA an18Aspect->MakeText3d(Standard_False);
			//OA an18Aspect->TextAspect()->SetHeight(0.3);
			//OA an18Aspect->ArrowAspect()->SetLength(0.2);
			//OA an18Aspect->MakeTextShaded(Standard_True);
			//OA an18Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an18Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an18Aspect->SetArrowTailSize(0.3);
			//OA an18Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a18LenDim->SetTextPosition(Point18);
			//OA a18LenDim->SetDimensionAspect(an18Aspect);
			//_geoWindow->getAisContext()->Display(a18LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a18LenDim);
			//OA _a18LenDim = a18LenDim;

			//OA Handle(AIS_LengthDimension) a12LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge1), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an12Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t2Value = a12LenDim->GetValue();
			//OA TCollection_AsciiString t2text = "DIM3 = ";
			//OA an12Aspect->SetValueStringFormat(t2text + t2Value);
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

			//OA Handle(AIS_LengthDimension) a36LenDim = new AIS_LengthDimension(Point3, Point6, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an36Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t1Value = a36LenDim->GetValue();
			//OA TCollection_AsciiString t1text = "DIM4 = ";
			//OA an36Aspect->SetValueStringFormat(t1text + t1Value);
			//OA an36Aspect->MakeArrows3d(Standard_False);
			//OA an36Aspect->MakeText3d(Standard_False);
			//OA an36Aspect->TextAspect()->SetHeight(0.3);
			//OA an36Aspect->ArrowAspect()->SetLength(0.2);
			//OA an36Aspect->MakeTextShaded(Standard_True);
			//OA an36Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an36Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an36Aspect->SetArrowTailSize(0.3);
			//OA an36Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a36LenDim->SetTextPosition(Point36);
			//OA a36LenDim->SetDimensionAspect(an36Aspect);
			//_geoWindow->getAisContext()->Display(a36LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a36LenDim);
			//OA _a36LenDim = a36LenDim;

			//OA Handle(AIS_LengthDimension) E84LenDim = new AIS_LengthDimension(Edge4, Edge8, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an84Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValue = E84LenDim->GetValue();
			//OA TCollection_AsciiString btext = "DIM2 = ";
			//OA an84Aspect->SetValueStringFormat(btext + bValue);
			//OA an84Aspect->MakeArrows3d(Standard_False);
			//OA an84Aspect->MakeText3d(Standard_False);
			//OA an84Aspect->TextAspect()->SetHeight(0.3);
			//OA an84Aspect->ArrowAspect()->SetLength(0.2);
			//OA an84Aspect->MakeTextShaded(Standard_True);
			//OA an84Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an84Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an84Aspect->SetArrowTailSize(0.3);
			//OA an84Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA E84LenDim->SetTextPosition(PointE84);
			//OA E84LenDim->SetDimensionAspect(an84Aspect);
			//_geoWindow->getAisContext()->Display(E84LenDim, true);
			//OA beamView::_nameAisPair.insert(name, E84LenDim);
			//OA _E84LenDim = E84LenDim;
			//求抗扭截面系数
			double ti1, bi1, Ci1 = 1.0 / 3.0;

			bi1 = a;
			ti1 = t2;

			double It1 = Ci1 * bi1*pow(ti1, 3);

			double ti2, bi2, Ci2 = 1.0 / 3.0;

			bi2 = b - t2;
			ti2 = t1;
			double It2 = Ci2 * bi2*pow(ti2, 3);
			double It = It1 + It2 + 1.0 / 3.0*t2*pow(t1, 3) / 2;
			_data.push_back(It);
			//OA _data.push_back(shearCenter.Y());
			//OA _data.push_back(shearCenter.X());
			double SFZ = (a - t1)*t2 / _data.at(0);
			double SFY = (b - t2)*t1 / _data.at(0);
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

