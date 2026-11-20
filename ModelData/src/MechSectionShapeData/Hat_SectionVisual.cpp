#include "Hat_SectionVisual.h"
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
//OA #include <GProp_PrincipalProps.hxx>
//OA #include <gp_Lin2d.hxx>
namespace MechSectionShapeData
{
	Hat_SectionVisual::Hat_SectionVisual(QString name, int id)
		: mSectionShapeBase(Sec_Hat,name,id)
	{

		_para.push_back(DIM1);
		_para.push_back(DIM2);
		_para.push_back(DIM3);
		_para.push_back(DIM4);
	}

	Hat_SectionVisual::~Hat_SectionVisual()
	{
	}
	Hat_SectionVisual* Hat_SectionVisual::clone() const
	{
		Hat_SectionVisual* newSec = new Hat_SectionVisual(_sectionShapeName, _id);

		newSec->DIM1 = DIM1;
		newSec->DIM2 = DIM2;
		newSec->DIM3 = DIM3;
		newSec->DIM4 = DIM4;


		newSec->_shearMarker = _shearMarker;
		newSec->_a19LenDim = _a19LenDim;
		newSec->_a812LenDim = _a812LenDim;
		newSec->_a112LenDim = _a112LenDim;
		newSec->_P23LenDim = _P23LenDim;
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
	void Hat_SectionVisual::CreateHatSec()
	{
		try
		{
			double _Y1 =  DIM1 / 2, _X1 = DIM3 / 2, _Z1 = 0;
			double _Y2 = -DIM1 / 2 + DIM2, _X2 = DIM3 / 2, _Z2 = 0;
			double _Y3 = -DIM1 / 2 + DIM2, _X3 = DIM3 / 2+DIM4, _Z3 = 0;
			double _Y4 = -DIM1 / 2 , _X4 = DIM3 / 2 + DIM4, _Z4 = 0;
			double _Y5 = -DIM1 / 2 , _X5 = DIM3 / 2 - DIM2, _Z5 = 0;
			double _Y6 =  DIM1 / 2-DIM2, _X6 = DIM3 / 2 - DIM2, _Z6 = 0;


			//OA gp_Pnt Point1(_X1, _Y1, _Z1); gp_Pnt Point12(-_X1, _Y1, _Z1);
			//OA gp_Pnt Point2(_X2, _Y2, _Z2); gp_Pnt Point11(-_X2, _Y2, _Z2);
			//OA gp_Pnt Point3(_X3, _Y3, _Z3); gp_Pnt Point10(-_X3, _Y3, _Z3);
			//OA gp_Pnt Point4(_X4, _Y4, _Z4); gp_Pnt Point9(-_X4, _Y4, _Z3);
			//OA gp_Pnt Point5(_X5, _Y5, _Z5); gp_Pnt Point8(-_X5, _Y5, _Z3);
			//OA gp_Pnt Point6(_X6, _Y6, _Z6); gp_Pnt Point7(-_X5, _Y6, _Z3);

			//OA gp_Pnt PointDIM1Pos(-DIM3 / 2.0 - DIM4 - 0.5, DIM1 / 2.0 + 0.5, _Z3);
			//OA gp_Pnt PointDIM2Pos(-DIM3 / 2.0 + DIM2/2.0, 0, _Z3);
			//OA gp_Pnt PointDIM3Pos(0, DIM1 / 2.0 + 0.5, _Z3);
			//OA gp_Pnt PointDIM4Pos(DIM3 / 2.0 + DIM4 / 2.0, _Y2+0.5, _Z3);
			
			//OA gp_Pnt shearCenter(0, (_Y1 + _Y2) / 2, _Z3);


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
			QString name = mPBeam::getInstance()->setDefaultNameForBeam(Sec_Hat);

			/*emit AddSecSignal(name);
			emit showParaOnPanel(_para, name);*/
			//OA beamView::_nameAisPair.insert(name, aisshape);
			//OA beamView::_secNameAisPair.insert(name, aisshape);
			mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());
		/*	{
				gp_Pnt Point71(0, _Y6, _Z6); gp_Pnt Point81(0, _Y1, _Z3);*/

			//	//计算对称面的形心惯性主轴
			//	TopoDS_Edge Edge1 = BRepBuilderAPI_MakeEdge(Point1, Point2);
			//	TopoDS_Edge Edge2 = BRepBuilderAPI_MakeEdge(Point2, Point3);
			//	TopoDS_Edge Edge3 = BRepBuilderAPI_MakeEdge(Point3, Point4);
			//	TopoDS_Edge Edge4 = BRepBuilderAPI_MakeEdge(Point4, Point5);
			//	TopoDS_Edge Edge5 = BRepBuilderAPI_MakeEdge(Point5, Point6);
			//	TopoDS_Edge Edge6 = BRepBuilderAPI_MakeEdge(Point6, Point71);
			//	TopoDS_Edge Edge7 = BRepBuilderAPI_MakeEdge(Point71, Point81);
			//	TopoDS_Edge Edge8 = BRepBuilderAPI_MakeEdge(Point81, Point1);
			//	auto TSecWire1 = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//	TopoDS_Wire TSecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6, Edge7, Edge8);
			//	TSecWire1.Add(TSecWire2);
			//	TopoDS_Shape TSecFace = BRepBuilderAPI_MakeFace(TSecWire1);
			//	GProp_GProps System;
			//	BRepGProp::SurfaceProperties(TSecFace, System);
			//	auto centerPnt = System.CentreOfMass();
			//	auto mPPS = System.PrincipalProperties();
			//	auto f1 = mPPS.FirstAxisOfInertia();
			//	auto f2 = mPPS.SecondAxisOfInertia();
			//	auto f3 = mPPS.ThirdAxisOfInertia();
			//	gp_Lin2d mLine2d(gp_Pnt2d(centerPnt.X(), centerPnt.Y()),gp_Vec2d(f3.X(), f3.Y()));

			//	
			//	Handle(AIS_Shape) aisshape = new AIS_Shape(TSecFace);
			//	BaseshowBeamSec(aisshape);
			//}
			//OA getDataFromBeam(name, TSecFace);
			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(shearCenter.X(), shearCenter.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//_geoWindow->getAisContext()->Display(shearMarker, Standard_False);
			//OA beamView::_nameAisPair.insert(name, shearMarker);
			//OA _shearMarker = shearMarker;

			//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();


			//OA Handle(AIS_LengthDimension) a19LenDim = new AIS_LengthDimension(Edge8,Edge12, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an19Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString aValue = a19LenDim->GetValue();
			//OA TCollection_AsciiString atext = "DIM1 = ";
			//OA an19Aspect->SetValueStringFormat(atext + aValue);
			//OA an19Aspect->MakeArrows3d(Standard_False);
			//OA an19Aspect->MakeText3d(Standard_False);
			//OA an19Aspect->TextAspect()->SetHeight(0.3);
			//OA an19Aspect->ArrowAspect()->SetLength(0.2);
			//OA an19Aspect->MakeTextShaded(Standard_True);
			//OA an19Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an19Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an19Aspect->SetArrowTailSize(0.3);
			//OA an19Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a19LenDim->SetTextPosition(PointDIM1Pos);
			//OA a19LenDim->SetDimensionAspect(an19Aspect);
			//_geoWindow->getAisContext()->Display(a19LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a19LenDim);
			//OA _a19LenDim = a19LenDim;

			//OA Handle(AIS_LengthDimension) a812LenDim = new AIS_LengthDimension(Edge7,Edge11, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an812Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t2Value = a812LenDim->GetValue();
			//OA TCollection_AsciiString t2text = "DIM2 = ";
			//OA an812Aspect->SetValueStringFormat(t2text + t2Value);
			//OA an812Aspect->MakeArrows3d(Standard_False);
			//OA an812Aspect->MakeText3d(Standard_False);
			//OA an812Aspect->TextAspect()->SetHeight(0.3);
			//OA an812Aspect->ArrowAspect()->SetLength(0.2);
			//OA an812Aspect->MakeTextShaded(Standard_True);
			//OA an812Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an812Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an812Aspect->SetArrowTailSize(0.3);
			//OA an812Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a812LenDim->SetTextPosition(PointDIM2Pos);
			//OA a812LenDim->SetDimensionAspect(an812Aspect);
			//_geoWindow->getAisContext()->Display(a812LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a812LenDim);
			//OA _a812LenDim = a812LenDim;

			//OA Handle(AIS_LengthDimension) a112LenDim = new AIS_LengthDimension(Point1, Point12, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an112Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t1Value = a112LenDim->GetValue();
			//OA TCollection_AsciiString t1text = "DIM3 = ";
			//OA an112Aspect->SetValueStringFormat(t1text + t1Value);
			//OA an112Aspect->MakeArrows3d(Standard_False);
			//OA an112Aspect->MakeText3d(Standard_False);
			//OA an112Aspect->TextAspect()->SetHeight(0.3);
			//OA an112Aspect->ArrowAspect()->SetLength(0.2);
			//OA an112Aspect->MakeTextShaded(Standard_True);
			//OA an112Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an112Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an112Aspect->SetArrowTailSize(0.3);
			//OA an112Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a112LenDim->SetTextPosition(PointDIM3Pos);
			//OA a112LenDim->SetDimensionAspect(an112Aspect);
			//_geoWindow->getAisContext()->Display(a112LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a112LenDim);
			//OA _a112LenDim = a112LenDim;

			//OA Handle(AIS_LengthDimension) P23LenDim = new AIS_LengthDimension(Point2, Point3, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an23Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValue = P23LenDim->GetValue();
			//OA TCollection_AsciiString btext = "DIM4 = ";
			//OA an23Aspect->SetValueStringFormat(btext + bValue);
			//OA an23Aspect->MakeArrows3d(Standard_False);
			//OA an23Aspect->MakeText3d(Standard_False);
			//OA an23Aspect->TextAspect()->SetHeight(0.3);
			//OA an23Aspect->ArrowAspect()->SetLength(0.2);
			//OA an23Aspect->MakeTextShaded(Standard_True);
			//OA an23Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an23Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an23Aspect->SetArrowTailSize(0.3);
			//OA an23Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA P23LenDim->SetTextPosition(PointDIM4Pos);
			//OA P23LenDim->SetDimensionAspect(an23Aspect);
			//_geoWindow->getAisContext()->Display(P23LenDim, true);
			//OA beamView::_nameAisPair.insert(name, P23LenDim);
			//OA _P23LenDim = P23LenDim;

			double H1 = DIM1;
			double W1 = DIM3;
			double T = DIM2;
			double H2 = H1 - 2 * T;
			double W2 = DIM4;
			double W3 = W1 - 2.0 * T;
			area = H1 * T * 2 + W3 * T + W2 * T * 2;
			double Zc = 0.0;
			double Yc = ((0.5 * H2 + 0.5 * T) * W3 * T - (0.5 * H2 + 0.5 * T) * (W2 * T) * 2) / area;
			Iz = W1 / 3.0 * (pow(0.5 * H1 - Yc, 3) + pow(0.5 * H1 + Yc, 3)) - W3 / 3.0 * (pow(0.5 * H2 - Yc, 3) + pow(0.5 * H2 + T + Yc, 3))
				+ W2 / 3.0 * (pow(-0.5 * H2 - Yc, 3) + pow(0.5 * H2 + Yc + T, 3)) * 2;
			Iy = H1 * W1 * W1 * W1 / 12.0 - (H2 + T) * W3 * W3 * W3 / 12.0 + T / 3.0 * (pow(W2 + 0.5 * W1, 3) - pow(0.5 * W1, 3)) * 2;
			Iyz = 0.0;
			//扭转常数
			/*double beta1 = gettorsioncoefficient(H1, T);
			double beta2 = gettorsioncoefficient(W3, T);
			double beta3 = gettorsioncoefficient(W2, T);*/
			J = 1.0 / 3.0 * (H1 * T * T * T * 2.0 + W3 * T * T * T + 2.0 * W2 * T * T * T);
			//J = beta1 * H1 * T * T * T * 2 + beta2 * W3 * T * T * T + beta3 * 2.0 * W2 * T * T * T;
			//有效剪切面积
			K1 = (H1 - 2.0 * T) * 2.0 * T / area;//K1与hypermesh不一致，但结果一致
			K2 = W2 * T * 2.0 + W3 * T / area;
			_data.push_back(J);//抗扭截面系数
			//OA _data.push_back(shearCenter.Y());//剪切中心
			//OA _data.push_back(shearCenter.X());//剪切中心
			_data.push_back(K1);//截切系数
			_data.push_back(K2);//剪切系数

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
			//emit showDataOnPanel(_data);
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
		}
	
		
	}

	void Hat_SectionVisual::ReBuildHatSec(QList<double> para)
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




			double _Y1 = DIM1 / 2, _X1 = DIM3 / 2, _Z1 = 0;
			double _Y2 = -DIM1 / 2 + DIM2, _X2 = DIM3 / 2, _Z2 = 0;
			double _Y3 = -DIM1 / 2 + DIM2, _X3 = DIM3 / 2 + DIM4, _Z3 = 0;
			double _Y4 = -DIM1 / 2, _X4 = DIM3 / 2 + DIM4, _Z4 = 0;
			double _Y5 = -DIM1 / 2, _X5 = DIM3 / 2 - DIM2, _Z5 = 0;
			double _Y6 = DIM1 / 2 - DIM2, _X6 = DIM3 / 2 - DIM2, _Z6 = 0;


			//OA gp_Pnt Point1(_X1, _Y1, _Z1); gp_Pnt Point12(-_X1, _Y1, _Z1);
			//OA gp_Pnt Point2(_X2, _Y2, _Z2); gp_Pnt Point11(-_X2, _Y2, _Z2);
			//OA gp_Pnt Point3(_X3, _Y3, _Z3); gp_Pnt Point10(-_X3, _Y3, _Z3);
			//OA gp_Pnt Point4(_X4, _Y4, _Z4); gp_Pnt Point9(-_X4, _Y4, _Z3);
			//OA gp_Pnt Point5(_X5, _Y5, _Z5); gp_Pnt Point8(-_X5, _Y5, _Z3);
			//OA gp_Pnt Point6(_X6, _Y6, _Z6); gp_Pnt Point7(-_X5, _Y6, _Z3);

			//OA gp_Pnt PointDIM1Pos(-DIM3 / 2.0 - DIM4 - 0.5, DIM1 / 2.0 + 0.5, _Z3);
			//OA gp_Pnt PointDIM2Pos(-DIM3 / 2.0 + DIM2 / 2.0, 0, _Z3);
			//OA gp_Pnt PointDIM3Pos(0, DIM1 / 2.0 + 0.5, _Z3);
			//OA gp_Pnt PointDIM4Pos(DIM3 / 2.0 + DIM4 / 2.0, _Y2 + 0.5, _Z3);

			//OA gp_Pnt shearCenter(0, (_Y1 + _Y2) / 2, _Z3);
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

			//OA getDataFromBeam(name, TSecFace);

			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(shearCenter.X(), shearCenter.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//_geoWindow->getAisContext()->Display(shearMarker, Standard_False);
			//OA beamView::_nameAisPair.insert(name, shearMarker);
			//OA _shearMarker = shearMarker;


			//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();




			//OA Handle(AIS_LengthDimension) a19LenDim = new AIS_LengthDimension(Edge8, Edge12, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an19Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString aValue = a19LenDim->GetValue();
			//OA TCollection_AsciiString atext = "DIM1 = ";
			//OA an19Aspect->SetValueStringFormat(atext + aValue);
			//OA an19Aspect->MakeArrows3d(Standard_False);
			//OA an19Aspect->MakeText3d(Standard_False);
			//OA an19Aspect->TextAspect()->SetHeight(0.3);
			//OA an19Aspect->ArrowAspect()->SetLength(0.2);
			//OA an19Aspect->MakeTextShaded(Standard_True);
			//OA an19Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an19Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an19Aspect->SetArrowTailSize(0.3);
			//OA an19Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a19LenDim->SetTextPosition(PointDIM1Pos);
			//OA a19LenDim->SetDimensionAspect(an19Aspect);
			//_geoWindow->getAisContext()->Display(a19LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a19LenDim);
			//OA _a19LenDim = a19LenDim;


			//OA Handle(AIS_LengthDimension) a812LenDim = new AIS_LengthDimension(Edge7, Edge11, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an812Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t2Value = a812LenDim->GetValue();
			//OA TCollection_AsciiString t2text = "DIM2 = ";
			//OA an812Aspect->SetValueStringFormat(t2text + t2Value);
			//OA an812Aspect->MakeArrows3d(Standard_False);
			//OA an812Aspect->MakeText3d(Standard_False);
			//OA an812Aspect->TextAspect()->SetHeight(0.3);
			//OA an812Aspect->ArrowAspect()->SetLength(0.2);
			//OA an812Aspect->MakeTextShaded(Standard_True);
			//OA an812Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an812Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an812Aspect->SetArrowTailSize(0.3);
			//OA an812Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a812LenDim->SetTextPosition(PointDIM2Pos);
			//OA a812LenDim->SetDimensionAspect(an812Aspect);
			//_geoWindow->getAisContext()->Display(a812LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a812LenDim);
			//OA _a812LenDim = a812LenDim;

			//OA Handle(AIS_LengthDimension) a112LenDim = new AIS_LengthDimension(Point1, Point12, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an112Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t1Value = a112LenDim->GetValue();
			//OA TCollection_AsciiString t1text = "DIM3 = ";
			//OA an112Aspect->SetValueStringFormat(t1text + t1Value);
			//OA an112Aspect->MakeArrows3d(Standard_False);
			//OA an112Aspect->MakeText3d(Standard_False);
			//OA an112Aspect->TextAspect()->SetHeight(0.3);
			//OA an112Aspect->ArrowAspect()->SetLength(0.2);
			//OA an112Aspect->MakeTextShaded(Standard_True);
			//OA an112Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an112Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an112Aspect->SetArrowTailSize(0.3);
			//OA an112Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a112LenDim->SetTextPosition(PointDIM3Pos);
			//OA a112LenDim->SetDimensionAspect(an112Aspect);
			//_geoWindow->getAisContext()->Display(a112LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a112LenDim);
			//OA _a112LenDim = a112LenDim;

			//OA Handle(AIS_LengthDimension) P23LenDim = new AIS_LengthDimension(Point2, Point3, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an23Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValue = P23LenDim->GetValue();
			//OA TCollection_AsciiString btext = "DIM4 = ";
			//OA an23Aspect->SetValueStringFormat(btext + bValue);
			//OA an23Aspect->MakeArrows3d(Standard_False);
			//OA an23Aspect->MakeText3d(Standard_False);
			//OA an23Aspect->TextAspect()->SetHeight(0.3);
			//OA an23Aspect->ArrowAspect()->SetLength(0.2);
			//OA an23Aspect->MakeTextShaded(Standard_True);
			//OA an23Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an23Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an23Aspect->SetArrowTailSize(0.3);
			//OA an23Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA P23LenDim->SetTextPosition(PointDIM4Pos);
			//OA P23LenDim->SetDimensionAspect(an23Aspect);
			//_geoWindow->getAisContext()->Display(P23LenDim, true);
			//OA beamView::_nameAisPair.insert(name, P23LenDim);
			//OA _P23LenDim = P23LenDim;

			double H1 = DIM1;
			double W1 = DIM3;
			double T = DIM2;
			double H2 = H1 - 2 * T;
			double W2 = DIM4;
			double W3 = W1 - 2.0 * T;
			area = H1 * T * 2 + W3 * T + W2 * T * 2;
			double Zc = 0.0;
			double Yc = ((0.5 * H2 + 0.5 * T) * W3 * T - (0.5 * H2 + 0.5 * T) * (W2 * T) * 2) / area;
			Iz = W1 / 3.0 * (pow(0.5 * H1 - Yc, 3) + pow(0.5 * H1 + Yc, 3)) - W3 / 3.0 * (pow(0.5 * H2 - Yc, 3) + pow(0.5 * H2 + T + Yc, 3))
				+ W2 / 3.0 * (pow(-0.5 * H2 - Yc, 3) + pow(0.5 * H2 + Yc + T, 3)) * 2;
			Iy = H1 * W1 * W1 * W1 / 12.0 - (H2 + T) * W3 * W3 * W3 / 12.0 + T / 3.0 * (pow(W2 + 0.5 * W1, 3) - pow(0.5 * W1, 3)) * 2;
			Iyz = 0.0;
			//扭转常数
			/*double beta1 = gettorsioncoefficient(H1, T);
			double beta2 = gettorsioncoefficient(W3, T);
			double beta3 = gettorsioncoefficient(W2, T);*/
			J = 1.0 / 3.0 * (H1 * T * T * T * 2.0 + W3 * T * T * T + 2.0 * W2 * T * T * T);
			//J = beta1 * H1 * T * T * T * 2 + beta2 * W3 * T * T * T + beta3 * 2.0 * W2 * T * T * T;
			//有效剪切面积
			K1 = (H1 - 2.0 * T) * 2.0 * T/ area;//K1与hypermesh不一致，但结果一致
			K2 = W2 * T * 2.0 + W3 * T / area;
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

