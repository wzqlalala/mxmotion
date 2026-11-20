#include "Cross_SectionVisual.h"
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
	Cross_SectionVisual::Cross_SectionVisual(QString name, int id)
		: MechSectionShapeData::mSectionShapeBase(Sec_Cross,name,id)
	{

		_para.push_back(0.5*DIM1);
		_para.push_back(DIM2);
		_para.push_back(DIM3);
		_para.push_back(DIM4);
	}

	Cross_SectionVisual::~Cross_SectionVisual()
	{
	}
	Cross_SectionVisual* Cross_SectionVisual::clone() const
	{
		Cross_SectionVisual* newSec = new Cross_SectionVisual(_sectionShapeName, _id);

		newSec->DIM1 = DIM1;
		newSec->DIM2 = DIM2;
		newSec->DIM3 = DIM3;
		newSec->DIM4 = DIM4;


		newSec->_shearMarker = _shearMarker;
		newSec->_a1012LenDim = _a1012LenDim;
		newSec->_E68LenDim = _E68LenDim;
		newSec->_a17LenDim = _a17LenDim;
		newSec->_a911LenDim = _a911LenDim;
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
	void Cross_SectionVisual::CreateCrossSec()
	{
		try
		{
			double _Y1 = DIM3 / 2, _X1 =  - DIM2 / 2.0, _Z1 = 0;
			double _Y2 = DIM3 / 2, _X2 = DIM2 / 2, _Z2 = 0;
			double _Y3 = DIM4 / 2, _X3 = DIM2 / 2, _Z3 = 0;
			double _Y4 = DIM4 / 2, _X4 = (DIM1 + DIM2) / 2.0, _Z4 = 0;

			//OA gp_Pnt Point1(_X1, _Y1, _Z1); gp_Pnt Point8(_X1, -_Y1, _Z1);
			//OA gp_Pnt Point2(_X2, _Y2, _Z2); gp_Pnt Point7(_X2, -_Y2, _Z2);
			//OA gp_Pnt Point3(_X3, _Y3, _Z3); gp_Pnt Point6(_X3, -_Y3, _Z3); gp_Pnt Point9(-_X3, -_Y3, _Z3); gp_Pnt Point12(-_X3, _Y3, _Z3);
			//OA gp_Pnt Point4(_X4, _Y4, _Z4); gp_Pnt Point11(-_X4, _Y4, _Z4);
			//OA gp_Pnt Point5(_X4, -_Y4, _Z4); gp_Pnt Point10(-_X4, -_Y4, _Z4);


			//OA gp_Pnt PointDIM1Pos(-(0.5*DIM1 + DIM2) / 2.0, _Y1 + 0.5, _Z3);
			//OA gp_Pnt PointDIM2Pos(0, -DIM3/2.0-0.5, _Z3);
			//OA gp_Pnt PointDIM3Pos(DIM2 / 2.0 + 1, 0, _Z3);
			//OA gp_Pnt PointDIM4Pos(-(DIM1 + DIM2) / 2.0, 0, _Z3);

			//OA gp_Pnt shearCenter(0, 0, _Z3);

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


			//OA auto TSecWire1 = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//OA TopoDS_Wire TSecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6, Edge7, Edge8);
			//OA TopoDS_Wire TSecWire3 = BRepBuilderAPI_MakeWire(Edge9, Edge10, Edge11, Edge12);
			//OA TSecWire1.Add(TSecWire2);
			//OA TSecWire1.Add(TSecWire3);
			//OA TopoDS_Shape TSecFace = BRepBuilderAPI_MakeFace(TSecWire1);
			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(TSecFace);
			//OA BaseshowBeamSec(aisshape);
			//OA _aisshape = aisshape;
			QString name = mPBeam::getInstance()->setDefaultNameForBeam(Sec_Cross);

			//emit AddSecSignal(name);
			//emit showParaOnPanel(_para, name);
			//OA beamView::_nameAisPair.insert(name, aisshape);
			//OA beamView::_secNameAisPair.insert(name, aisshape);
			mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());
			//OA getDataFromBeam(name, TSecFace);
			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(shearCenter.X(), shearCenter.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			/*_geoWindow->getAisContext()->Display(shearMarker, Standard_False);*/
			//OA beamView::_nameAisPair.insert(name, shearMarker);
			//OA _shearMarker = shearMarker;
			//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();


			//OA Handle(AIS_LengthDimension) a1012LenDim = new AIS_LengthDimension(Edge10,Edge12, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an1012Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString aValue = a1012LenDim->GetValue();
			//OA TCollection_AsciiString atext = "0.5*DIM1 = ";
			//OA an1012Aspect->SetValueStringFormat(atext + aValue);
			//OA an1012Aspect->MakeArrows3d(Standard_False);
			//OA an1012Aspect->MakeText3d(Standard_False);
			//OA an1012Aspect->TextAspect()->SetHeight(0.3);
			//OA an1012Aspect->ArrowAspect()->SetLength(0.2);
			//OA an1012Aspect->MakeTextShaded(Standard_True);
			//OA an1012Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an1012Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an1012Aspect->SetArrowTailSize(0.3);
			//OA an1012Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a1012LenDim->SetTextPosition(PointDIM1Pos);
			//OA a1012LenDim->SetDimensionAspect(an1012Aspect);
			//_geoWindow->getAisContext()->Display(a1012LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a1012LenDim);
			//OA _a1012LenDim = a1012LenDim;
			//OA Handle(AIS_LengthDimension) E68LenDim = new AIS_LengthDimension(Point7, Point8, aPlane->Pln());
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
			//OA E68LenDim->SetTextPosition(PointDIM2Pos);
			//OA E68LenDim->SetDimensionAspect(an68Aspect);
			//_geoWindow->getAisContext()->Display(E68LenDim, true);
			//OA beamView::_nameAisPair.insert(name, E68LenDim);
			//OA _E68LenDim = E68LenDim;


			//OA Handle(AIS_LengthDimension) a17LenDim = new AIS_LengthDimension(Point2, Point7, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an17Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t2Value = a17LenDim->GetValue();
			//OA TCollection_AsciiString t2text = "DIM3 = ";
			//OA an17Aspect->SetValueStringFormat(t2text + t2Value);
			//OA an17Aspect->MakeArrows3d(Standard_False);
			//OA an17Aspect->MakeText3d(Standard_False);
			//OA an17Aspect->TextAspect()->SetHeight(0.3);
			//OA an17Aspect->ArrowAspect()->SetLength(0.2);
			//OA an17Aspect->MakeTextShaded(Standard_True);
			//OA an17Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an17Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an17Aspect->SetArrowTailSize(0.3);
			//OA an17Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a17LenDim->SetTextPosition(PointDIM3Pos);
			//OA a17LenDim->SetDimensionAspect(an17Aspect);
			//_geoWindow->getAisContext()->Display(a17LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a17LenDim);

			//OA Handle(AIS_LengthDimension) a911LenDim = new AIS_LengthDimension(Point10, Point11, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an911Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t1Value = a911LenDim->GetValue();
			//OA TCollection_AsciiString t1text = "DIM4 = ";
			//OA an911Aspect->SetValueStringFormat(t1text + t1Value);
			//OA an911Aspect->MakeArrows3d(Standard_False);
			//OA an911Aspect->MakeText3d(Standard_False);
			//OA an911Aspect->TextAspect()->SetHeight(0.3);
			//OA an911Aspect->ArrowAspect()->SetLength(0.2);
			//OA an911Aspect->MakeTextShaded(Standard_True);
			//OA an911Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an911Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an911Aspect->SetArrowTailSize(0.3);
			//OA an911Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a911LenDim->SetTextPosition(PointDIM4Pos);
			//OA a911LenDim->SetDimensionAspect(an911Aspect);
			//_geoWindow->getAisContext()->Display(a911LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a911LenDim);
			//OA _a911LenDim = a911LenDim;

			double H = DIM3;
			double W = DIM1 + DIM2;
			double hi = DIM4;
			double wi = DIM2;
			area = wi * H + W * hi - wi * hi;
			double Yc = 0.0;
			double Zc = 0.0;
			Iz = H * H * H * wi / 12.0 + hi * hi * hi * W / 12.0 - hi * hi * hi * wi / 12.0;
			Iy = wi * wi * wi * H / 12.0 + W * W * W * hi / 12.0 - wi * wi * wi * hi / 12.0;
			Iyz = 0.0;
			//扭转常数
			// double beta1 = 0.0;
			// double beta2 = gettorsioncoefficient(H, wi);
			J = 1.0 / 3.0 *(H * wi * wi * wi + 2.0 * 0.5 * DIM1 * hi * hi * hi);
			//有效剪切面积
			// K1 = (H - hi) * wi;
			// K2 = (W - wi) * hi;
			K1 = (5.0 / 6.0) * H * wi / area;
			K2 = (5.0 / 6.0) * W * hi / area;
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
			//emit showDataOnPanel(_data);
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
		}
	
		
	}

	void Cross_SectionVisual::ReBuildCrossSec(QList<double> para)
	{
		try
		{
			DIM1 = para.at(1) * 2;
			DIM2 = para.at(2);
			DIM3 = para.at(3);
			DIM4 = para.at(4);
			//_geoWindow->getAisContext()->EraseAll(true);
			para.removeFirst();
			_para = para;

			double _Y1 = DIM3 / 2, _X1 = -DIM2 / 2.0, _Z1 = 0;
			double _Y2 = DIM3 / 2, _X2 = DIM2 / 2, _Z2 = 0;
			double _Y3 = DIM4 / 2, _X3 = DIM2 / 2, _Z3 = 0;
			double _Y4 = DIM4 / 2, _X4 = (DIM1 + DIM2) / 2.0, _Z4 = 0;

			//OA gp_Pnt Point1(_X1, _Y1, _Z1); gp_Pnt Point8(_X1, -_Y1, _Z1);
			//OA gp_Pnt Point2(_X2, _Y2, _Z2); gp_Pnt Point7(_X2, -_Y2, _Z2);
			//OA gp_Pnt Point3(_X3, _Y3, _Z3); gp_Pnt Point6(_X3, -_Y3, _Z3); gp_Pnt Point9(-_X3, -_Y3, _Z3); gp_Pnt Point12(-_X3, _Y3, _Z3);
			//OA gp_Pnt Point4(_X4, _Y4, _Z4); gp_Pnt Point11(-_X4, _Y4, _Z4);
			//OA gp_Pnt Point5(_X4, -_Y4, _Z4); gp_Pnt Point10(-_X4, -_Y4, _Z4);


			//OA gp_Pnt PointDIM1Pos(-(0.5*DIM1 + DIM2) / 2.0, _Y1 + 0.5, _Z3);
			//OA gp_Pnt PointDIM2Pos(0, -DIM3 / 2.0 - 0.5, _Z3);
			//OA gp_Pnt PointDIM3Pos(DIM2 / 2.0 + 1, 0, _Z3);
			//OA gp_Pnt PointDIM4Pos(-(DIM1 + DIM2) / 2.0, 0, _Z3);


			//OA gp_Pnt shearCenter(0, 0, _Z3);

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


			//OA auto TSecWire1 = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//OA TopoDS_Wire TSecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6, Edge7, Edge8);
			//OA TopoDS_Wire TSecWire3 = BRepBuilderAPI_MakeWire(Edge9, Edge10, Edge11, Edge12);
			//OA TSecWire1.Add(TSecWire2);
			//OA TSecWire1.Add(TSecWire3);
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


			//OA Handle(AIS_LengthDimension) a1012LenDim = new AIS_LengthDimension(Edge10, Edge12, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an1012Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString aValue = a1012LenDim->GetValue();
			//OA TCollection_AsciiString atext = "0.5*DIM1 = ";
			//OA an1012Aspect->SetValueStringFormat(atext + aValue);
			//OA an1012Aspect->MakeArrows3d(Standard_False);
			//OA an1012Aspect->MakeText3d(Standard_False);
			//OA an1012Aspect->TextAspect()->SetHeight(0.3);
			//OA an1012Aspect->ArrowAspect()->SetLength(0.2);
			//OA an1012Aspect->MakeTextShaded(Standard_True);
			//OA an1012Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an1012Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an1012Aspect->SetArrowTailSize(0.3);
			//OA an1012Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a1012LenDim->SetTextPosition(PointDIM1Pos);
			//OA a1012LenDim->SetDimensionAspect(an1012Aspect);
			//_geoWindow->getAisContext()->Display(a1012LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a1012LenDim);
			//OA _a1012LenDim = a1012LenDim;

			//OA Handle(AIS_LengthDimension) E68LenDim = new AIS_LengthDimension(Point7, Point8, aPlane->Pln());
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
			//OA E68LenDim->SetTextPosition(PointDIM2Pos);
			//OA E68LenDim->SetDimensionAspect(an68Aspect);
			//_geoWindow->getAisContext()->Display(E68LenDim, true);
			//OA beamView::_nameAisPair.insert(name, E68LenDim);
			//OA _E68LenDim = E68LenDim;

			//OA Handle(AIS_LengthDimension) a17LenDim = new AIS_LengthDimension(Point2, Point7, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an17Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t2Value = a17LenDim->GetValue();
			//OA TCollection_AsciiString t2text = "DIM3 = ";
			//OA an17Aspect->SetValueStringFormat(t2text + t2Value);
			//OA an17Aspect->MakeArrows3d(Standard_False);
			//OA an17Aspect->MakeText3d(Standard_False);
			//OA an17Aspect->TextAspect()->SetHeight(0.3);
			//OA an17Aspect->ArrowAspect()->SetLength(0.2);
			//OA an17Aspect->MakeTextShaded(Standard_True);
			//OA an17Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an17Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an17Aspect->SetArrowTailSize(0.3);
			//OA an17Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a17LenDim->SetTextPosition(PointDIM3Pos);
			//OA a17LenDim->SetDimensionAspect(an17Aspect);
			//_geoWindow->getAisContext()->Display(a17LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a17LenDim);
			//OA _a17LenDim = a17LenDim;

			//OA Handle(AIS_LengthDimension) a911LenDim = new AIS_LengthDimension(Point10, Point11, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an911Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t1Value = a911LenDim->GetValue();
			//OA TCollection_AsciiString t1text = "DIM4 = ";
			//OA an911Aspect->SetValueStringFormat(t1text + t1Value);
			//OA an911Aspect->MakeArrows3d(Standard_False);
			//OA an911Aspect->MakeText3d(Standard_False);
			//OA an911Aspect->TextAspect()->SetHeight(0.3);
			//OA an911Aspect->ArrowAspect()->SetLength(0.2);
			//OA an911Aspect->MakeTextShaded(Standard_True);
			//OA an911Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an911Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an911Aspect->SetArrowTailSize(0.3);
			//OA an911Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a911LenDim->SetTextPosition(PointDIM4Pos);
			//OA a911LenDim->SetDimensionAspect(an911Aspect);
			//_geoWindow->getAisContext()->Display(a911LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a911LenDim);
			//OA _a911LenDim = a911LenDim;

			double H = DIM3;
			double W = DIM1 + DIM2;
			double hi = DIM4;
			double wi = DIM2;
			area = wi * H + W * hi - wi * hi;
			double Yc = 0.0;
			double Zc = 0.0;
			Iz = H * H * H * wi / 12.0 + hi * hi * hi * W / 12.0 - hi * hi * hi * wi / 12.0;
			Iy = wi * wi * wi * H / 12.0 + W * W * W * hi / 12.0 - wi * wi * wi * hi / 12.0;
			Iyz = 0.0;
			//扭转常数
			// double beta1 = 0.0;
			// double beta2 = gettorsioncoefficient(H, wi);
			J = 1.0 / 3.0 *(H * wi * wi * wi + 2.0 * 0.5 * DIM1 * hi * hi * hi);
			//有效剪切面积
			// K1 = (H - hi) * wi;
			// K2 = (W - wi) * hi;
			K1 = (5.0 / 6.0) * H * wi / area;
			K2 = (5.0 / 6.0) * W * hi / area;
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
			//emit showDataOnPanel(_data);
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
		}
		
	}
}

