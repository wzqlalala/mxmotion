#include "Box1_SectionVisual.h"
//#include "gp_Pnt.hxx"
//#include "BRepBuilderAPI_MakeEdge.hxx"
//#include "BRepBuilderAPI_MakeFace.hxx"
//#include "BRepBuilderAPI_MakeWire.hxx"
//#include "TopoDS_Face.hxx"
//#include "TopoDS_Wire.hxx"

//#include <Prs3d_Drawer.hxx>
//#include <AIS_InteractiveContext.hxx>
//#include <AIS_LengthDimension.hxx>
//#include <BRepAlgoAPI_Cut.hxx>

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
	Box1_SectionVisual::Box1_SectionVisual(QString name, int id)
		: mSectionShapeBase(Sec_Box1,name,id)
	{

		_para.push_back(DIM1);
		_para.push_back(DIM2);
		_para.push_back(DIM3);
		_para.push_back(DIM4);
		_para.push_back(DIM5);
		_para.push_back(DIM6);
	}

	Box1_SectionVisual::~Box1_SectionVisual()
	{

	}
	Box1_SectionVisual* Box1_SectionVisual::clone() const
	{
		Box1_SectionVisual* newSec = new Box1_SectionVisual(_sectionShapeName, _id);

		newSec->DIM1 = DIM1;
		newSec->DIM2 = DIM2;
		newSec->DIM3 = DIM3;
		newSec->DIM4 = DIM4;
		newSec->DIM5 = DIM5;
		newSec->DIM6 = DIM6;

		newSec->_shearMarker = _shearMarker;
		newSec->_LenDim1 = _LenDim1;
		newSec->_aLenDim2 = _aLenDim2;
		newSec->_aLenDim3 = _aLenDim3;
		newSec->_LenDim4 = _LenDim4;
		newSec->_LenDim5 = _LenDim5;
		newSec->_LenDim6 = _LenDim6;
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
	void Box1_SectionVisual::CreateBox1Sec()
	{
		try
		{
			double _Y1 = DIM2 / 2, _X1 = -DIM1 / 2, _Z1 = 0;
			double _Y5 = DIM2 / 2 - DIM3, _X5 = -DIM1 / 2 + DIM6, _Z5 = 0;
			double _Y6 = DIM2 / 2 - DIM3, _X6 = DIM1 / 2 - DIM5, _Z6 = 0;
			double _Y7 = -DIM2 / 2 + DIM4, _X7 = DIM1 / 2 - DIM5, _Z7 = 0;
			double _Y8 = -DIM2 / 2 + DIM4, _X8 = -DIM1 / 2 + DIM6, _Z8 = 0;
			


			//OA gp_Pnt Point1(_X1, _Y1, _Z1);gp_Pnt Point2(-_X1, _Y1, _Z1); gp_Pnt Point3(-_X1, -_Y1, _Z1); gp_Pnt Point4(_X1, -_Y1, _Z1);
			//OA gp_Pnt Point5(_X5, _Y5, _Z5); gp_Pnt Point6(_X6, _Y6, _Z6); gp_Pnt Point7(_X7, _Y7, _Z7); gp_Pnt Point8(_X8, _Y8, _Z8);

			//OA gp_Pnt PointDIM1Pos(0, -DIM2 / 2.0 - 0.5, 0);
			//OA gp_Pnt PointDIM2Pos(-DIM1 / 2.0 - 0.5, 0, 0);
			//OA gp_Pnt PointDIM3Pos(-DIM1 / 2.0+DIM6 + 0.5, DIM2 / 2.0 - DIM3/2.0, 0);
			//OA gp_Pnt PointDIM4Pos(DIM1 / 2 - DIM5, -DIM1 / 2 + DIM4/2.0, 0);
			//OA gp_Pnt PointDIM5Pos(DIM1 / 2 - DIM5/2.0, DIM2 / 2.0 - DIM3 / 2.0, 0);
			//OA gp_Pnt PointDIM6Pos(-DIM1 / 2.0 + DIM6/2.0, -DIM2 / 2 + DIM4+0.5, 0);



			/*gp_Pnt Point2(_X2, _Y2, _Z2); gp_Pnt Point7(-_X2, _Y2, _Z2);
			gp_Pnt Point3(_X3, _Y3, _Z3); gp_Pnt Point4(_X3, -_Y3 - t2, _Z3); gp_Pnt Point5(-_X3, -_Y3 - t2, _Z3); gp_Pnt Point6(-_X3, _Y3, _Z3);
			gp_Pnt Point18(0, _Y1 + 0.5, _Z3); gp_Pnt Point12(_X1 - 0.5, (_Y1 + _Y2) / 2, _Z3); gp_Pnt Point36(0, _Y3 - 0.5, _Z3); gp_Pnt PointE84(-_X1, 0, _Z3);*/
			//OA gp_Pnt shearCenter(0, 0, 0);
			//OA TopoDS_Edge Edge1 = BRepBuilderAPI_MakeEdge(Point1, Point2);
			//OA TopoDS_Edge Edge2 = BRepBuilderAPI_MakeEdge(Point2, Point3);
			//OA TopoDS_Edge Edge3 = BRepBuilderAPI_MakeEdge(Point3, Point4);
			//OA TopoDS_Edge Edge4 = BRepBuilderAPI_MakeEdge(Point4, Point1);
			//OA TopoDS_Edge Edge5 = BRepBuilderAPI_MakeEdge(Point5, Point6);
			//OA TopoDS_Edge Edge6 = BRepBuilderAPI_MakeEdge(Point6, Point7);
			//OA TopoDS_Edge Edge7 = BRepBuilderAPI_MakeEdge(Point7, Point8);
			//OA TopoDS_Edge Edge8 = BRepBuilderAPI_MakeEdge(Point8, Point5);
			//OA auto TSecWire1 = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//OA TopoDS_Wire TSecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6, Edge7, Edge8);
			//OA TopoDS_Shape face1 = BRepBuilderAPI_MakeFace(TSecWire1);

			//OA TopoDS_Shape face2 = BRepBuilderAPI_MakeFace(TSecWire2);
			//OA TopoDS_Shape TSecFace = BRepAlgoAPI_Cut(face1, face2);

			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(TSecFace);
			//OA _aisshape = aisshape;
			//OA BaseshowBeamSec(aisshape);
			QString name = mPBeam::getInstance()->setDefaultNameForBeam(Sec_Box1);

			/*emit AddSecSignal(name);
			emit showParaOnPanel(_para, name);*/
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


			//OA Handle(AIS_LengthDimension) LenDim1 = new AIS_LengthDimension(Point3,Point4, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) anAspectDim1 = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString aValue = LenDim1->GetValue();
			//OA TCollection_AsciiString atext = "DIM1 = ";
			//OA anAspectDim1->SetValueStringFormat(atext + aValue);
			//OA anAspectDim1->MakeArrows3d(Standard_False);
			//OA anAspectDim1->MakeText3d(Standard_False);
			//OA anAspectDim1->TextAspect()->SetHeight(0.3);
			//OA anAspectDim1->ArrowAspect()->SetLength(0.2);
			//OA anAspectDim1->MakeTextShaded(Standard_True);
			//OA anAspectDim1->SetCommonColor(Quantity_NOC_BLACK);
			//OA anAspectDim1->MakeUnitsDisplayed(Standard_False);
			//OA anAspectDim1->SetArrowTailSize(0.3);
			//OA anAspectDim1->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA LenDim1->SetTextPosition(PointDIM1Pos);
			//OA LenDim1->SetDimensionAspect(anAspectDim1);
			//_geoWindow->getAisContext()->Display(LenDim1, true);
			//OA beamView::_nameAisPair.insert(name, LenDim1);
			//OA _LenDim1 = LenDim1;


			//OA Handle(AIS_LengthDimension) aLenDim2 = new AIS_LengthDimension(Point4,Point1, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) anAspectDim2 = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t2Value = aLenDim2->GetValue();
			//OA TCollection_AsciiString t2text = "DIM2 = ";
			//OA anAspectDim2->SetValueStringFormat(t2text + t2Value);
			//OA anAspectDim2->MakeArrows3d(Standard_False);
			//OA anAspectDim2->MakeText3d(Standard_False);
			//OA anAspectDim2->TextAspect()->SetHeight(0.3);
			//OA anAspectDim2->ArrowAspect()->SetLength(0.2);
			//OA anAspectDim2->MakeTextShaded(Standard_True);
			//OA anAspectDim2->SetCommonColor(Quantity_NOC_BLACK);
			//OA anAspectDim2->MakeUnitsDisplayed(Standard_False);
			//OA anAspectDim2->SetArrowTailSize(0.3);
			//OA anAspectDim2->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA aLenDim2->SetTextPosition(PointDIM2Pos);
			//OA aLenDim2->SetDimensionAspect(anAspectDim2);
			//_geoWindow->getAisContext()->Display(aLenDim2, true);
			//OA beamView::_nameAisPair.insert(name, aLenDim2);
			//OA _aLenDim2 = aLenDim2;



			//OA Handle(AIS_LengthDimension) aLenDim3 = new AIS_LengthDimension(Edge1, Edge5, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) anAspectDim3 = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t1Value = aLenDim3->GetValue();
			//OA TCollection_AsciiString t1text = "DIM3 = ";
			//OA anAspectDim3->SetValueStringFormat(t1text + t1Value);
			//OA anAspectDim3->MakeArrows3d(Standard_False);
			//OA anAspectDim3->MakeText3d(Standard_False);
			//OA anAspectDim3->TextAspect()->SetHeight(0.3);
			//OA anAspectDim3->ArrowAspect()->SetLength(0.2);
			//OA anAspectDim3->MakeTextShaded(Standard_True);
			//OA anAspectDim3->SetCommonColor(Quantity_NOC_BLACK);
			//OA anAspectDim3->MakeUnitsDisplayed(Standard_False);
			//OA anAspectDim3->SetArrowTailSize(0.3);
			//OA anAspectDim3->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA aLenDim3->SetTextPosition(PointDIM3Pos);
			//OA aLenDim3->SetDimensionAspect(anAspectDim3);
			//_geoWindow->getAisContext()->Display(aLenDim3, true);
			//OA beamView::_nameAisPair.insert(name, aLenDim3);
			//OA _aLenDim3 = aLenDim3;

			//OA Handle(AIS_LengthDimension) LenDim4 = new AIS_LengthDimension(Edge3, Edge7, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) anAspectDim4 = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValue = LenDim4->GetValue();
			//OA TCollection_AsciiString btext = "DIM4 = ";
			//OA anAspectDim4->SetValueStringFormat(btext + bValue);
			//OA anAspectDim4->MakeArrows3d(Standard_False);
			//OA anAspectDim4->MakeText3d(Standard_False);
			//OA anAspectDim4->TextAspect()->SetHeight(0.3);
			//OA anAspectDim4->ArrowAspect()->SetLength(0.2);
			//OA anAspectDim4->MakeTextShaded(Standard_True);
			//OA anAspectDim4->SetCommonColor(Quantity_NOC_BLACK);
			//OA anAspectDim4->MakeUnitsDisplayed(Standard_False);
			//OA anAspectDim4->SetArrowTailSize(0.3);
			//OA anAspectDim4->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA LenDim4->SetTextPosition(PointDIM4Pos);
			//OA LenDim4->SetDimensionAspect(anAspectDim4);
			//_geoWindow->getAisContext()->Display(LenDim4, true);
			//OA beamView::_nameAisPair.insert(name, LenDim4);
			//OA _LenDim4 = LenDim4;

			//OA Handle(AIS_LengthDimension) LenDim5 = new AIS_LengthDimension(Edge2, Edge6, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) anAspectDim5 = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValueDIM5 = LenDim5->GetValue();
			//OA TCollection_AsciiString btextDim5 = "DIM5 = ";
			//OA anAspectDim5->SetValueStringFormat(btextDim5 + bValueDIM5);
			//OA anAspectDim5->MakeArrows3d(Standard_False);
			//OA anAspectDim5->MakeText3d(Standard_False);
			//OA anAspectDim5->TextAspect()->SetHeight(0.3);
			//OA anAspectDim5->ArrowAspect()->SetLength(0.2);
			//OA anAspectDim5->MakeTextShaded(Standard_True);
			//OA anAspectDim5->SetCommonColor(Quantity_NOC_BLACK);
			//OA anAspectDim5->MakeUnitsDisplayed(Standard_False);
			//OA anAspectDim5->SetArrowTailSize(0.3);
			//OA anAspectDim5->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA LenDim5->SetTextPosition(PointDIM5Pos);
			//OA LenDim5->SetDimensionAspect(anAspectDim5);
			//_geoWindow->getAisContext()->Display(LenDim5, true);
			//OA beamView::_nameAisPair.insert(name, LenDim5);
			//OA _LenDim5 = LenDim5;

			//OA Handle(AIS_LengthDimension) LenDim6 = new AIS_LengthDimension(Edge4, Edge8, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) anAspectDim6 = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValueDim6 = LenDim6->GetValue();
			//OA TCollection_AsciiString btextDim6 = "DIM6 = ";
			//OA anAspectDim6->SetValueStringFormat(btextDim6 + bValueDim6);
			//OA anAspectDim6->MakeArrows3d(Standard_False);
			//OA anAspectDim6->MakeText3d(Standard_False);
			//OA anAspectDim6->TextAspect()->SetHeight(0.3);
			//OA anAspectDim6->ArrowAspect()->SetLength(0.2);
			//OA anAspectDim6->MakeTextShaded(Standard_True);
			//OA anAspectDim6->SetCommonColor(Quantity_NOC_BLACK);
			//OA anAspectDim6->MakeUnitsDisplayed(Standard_False);
			//OA anAspectDim6->SetArrowTailSize(0.3);
			//OA anAspectDim6->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA LenDim6->SetTextPosition(PointDIM6Pos);
			//OA LenDim6->SetDimensionAspect(anAspectDim6);
			//_geoWindow->getAisContext()->Display(LenDim6, true);
			//OA beamView::_nameAisPair.insert(name, LenDim6);
			//OA _LenDim6 = LenDim6;

			double H = DIM2;
			double W = DIM1;
			double TL = DIM6;
			double TR = DIM5;
			double TB = DIM4;
			double TT = DIM3;
			double wi = W - TL - TR;
			double hi = H - TT - TB;
			//截面参数
			area = H * (TL + TR) + (W - TL - TR) * (TT + TB);
			double Zc = ((0.5 * W - 0.5 * TR) * TR * (H - TT - TB) - (0.5 * W - 0.5 * TL) * ((H - TT - TB) * TL)) / area;
			double Yc = ((0.5 * H - 0.5 * TT) * ((W - TL - TR) * TT) - (0.5 * H - 0.5 * TB) * (W - TL - TR) * TB) / area;
			//积分的上下界
			double B = 0.5 * H - Yc;
			double A = -0.5 * H - Yc;
			double b = 0.5 * H - TT - Yc;
			double a = TB - 0.5 * H - Yc;
			double D = 0.5 * W - Zc;
			double C = -0.5 * W - Zc;
			double d = 0.5 * W - TR - Zc;
			double c = TL - 0.5 * W - Zc;
			//Iz和Iy
			Iz = W / 3.0 * (pow(B, 3) - pow(A, 3)) - wi / 3.0 * (pow(b, 3) - pow(a, 3));
			Iy = H / 3.0 * (pow(D, 3) - pow(C, 3)) - hi / 3.0 * (pow(d, 3) - pow(c, 3));
			//Iyz
			Iyz = (B * B - b * b) * (d * d - c * c) / 4.0 + (a * a - A * A) * (d * d - c * c) / 4.0 +
				(c * c - C * C) * (B * B - A * A) / 4.0 + (D * D - d * d) * (B * B - A * A) / 4.0;
			//自由扭转常数
			double ASTAR = (W - 0.5 * (TL + TR)) * (H - 0.5 * (TT + TB));
			double DEN = (H - 0.5 * (TT + TB)) * (1.0 / TL + 1.0 / TR) + (W - 0.5 * (TL + TR)) * (1.0 / TT + 1.0 / TB);
			J = 4.0 * ASTAR * ASTAR / DEN;
			
			//有效剪切面积
			K1 = (H - TT - TB) * (TL + TR) / area;
			K2 = (W - TL - TR) * (TT + TB) / area;

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

	void Box1_SectionVisual::ReBuildBox1Sec(QList<double> para)
	{
		try
		{
			DIM1 = para.at(1);
			DIM2 = para.at(2);
			DIM3 = para.at(3);
			DIM4 = para.at(4);
			DIM5 = para.at(5);
			DIM6 = para.at(6);
			//_geoWindow->getAisContext()->EraseAll(true);
			para.removeFirst();
			_para = para;


			double _Y1 = DIM2 / 2, _X1 = -DIM1 / 2, _Z1 = 0;
			double _Y5 = DIM2 / 2 - DIM3, _X5 = -DIM1 / 2 + DIM6, _Z5 = 0;
			double _Y6 = DIM2 / 2 - DIM3, _X6 = DIM1 / 2 - DIM5, _Z6 = 0;
			double _Y7 = -DIM2 / 2 + DIM4, _X7 = DIM1 / 2 - DIM5, _Z7 = 0;
			double _Y8 = -DIM2 / 2 + DIM4, _X8 = -DIM1 / 2 + DIM6, _Z8 = 0;



			//OA gp_Pnt Point1(_X1, _Y1, _Z1); gp_Pnt Point2(-_X1, _Y1, _Z1); gp_Pnt Point3(-_X1, -_Y1, _Z1); gp_Pnt Point4(_X1, -_Y1, _Z1);
			//OA gp_Pnt Point5(_X5, _Y5, _Z5); gp_Pnt Point6(_X6, _Y6, _Z6); gp_Pnt Point7(_X7, _Y7, _Z7); gp_Pnt Point8(_X8, _Y8, _Z8);

			//OA gp_Pnt PointDIM1Pos(0, -DIM2 / 2.0 - 0.5, 0);
			//OA gp_Pnt PointDIM2Pos(-DIM1 / 2.0 - 0.5, 0, 0);
			//OA gp_Pnt PointDIM3Pos(-DIM1 / 2.0 + DIM6 + 0.5, DIM2 / 2.0 - DIM3 / 2.0, 0);
			//OA gp_Pnt PointDIM4Pos(DIM1 / 2 - DIM5, -DIM1 / 2 + DIM4 / 2.0, 0);
			//OA gp_Pnt PointDIM5Pos(DIM1 / 2 - DIM5 / 2.0, DIM2 / 2.0 - DIM3 / 2.0, 0);
			//OA gp_Pnt PointDIM6Pos(-DIM1 / 2.0 + DIM6 / 2.0, -DIM2 / 2 + DIM4 + 0.5, 0);



			/*gp_Pnt Point2(_X2, _Y2, _Z2); gp_Pnt Point7(-_X2, _Y2, _Z2);
			gp_Pnt Point3(_X3, _Y3, _Z3); gp_Pnt Point4(_X3, -_Y3 - t2, _Z3); gp_Pnt Point5(-_X3, -_Y3 - t2, _Z3); gp_Pnt Point6(-_X3, _Y3, _Z3);
			gp_Pnt Point18(0, _Y1 + 0.5, _Z3); gp_Pnt Point12(_X1 - 0.5, (_Y1 + _Y2) / 2, _Z3); gp_Pnt Point36(0, _Y3 - 0.5, _Z3); gp_Pnt PointE84(-_X1, 0, _Z3);*/
			//OA gp_Pnt shearCenter(0, 0, 0);
			//OA TopoDS_Edge Edge1 = BRepBuilderAPI_MakeEdge(Point1, Point2);
			//OA TopoDS_Edge Edge2 = BRepBuilderAPI_MakeEdge(Point2, Point3);
			//OA TopoDS_Edge Edge3 = BRepBuilderAPI_MakeEdge(Point3, Point4);
			//OA TopoDS_Edge Edge4 = BRepBuilderAPI_MakeEdge(Point4, Point1);
			//OA TopoDS_Edge Edge5 = BRepBuilderAPI_MakeEdge(Point5, Point6);
			//OA TopoDS_Edge Edge6 = BRepBuilderAPI_MakeEdge(Point6, Point7);
			//OA TopoDS_Edge Edge7 = BRepBuilderAPI_MakeEdge(Point7, Point8);
			//OA TopoDS_Edge Edge8 = BRepBuilderAPI_MakeEdge(Point8, Point5);
			//OA auto TSecWire1 = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//OA TopoDS_Wire TSecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6, Edge7, Edge8);
			//OA TopoDS_Shape face1 = BRepBuilderAPI_MakeFace(TSecWire1);

			//OA TopoDS_Shape face2 = BRepBuilderAPI_MakeFace(TSecWire2);
			//OA TopoDS_Shape TSecFace = BRepAlgoAPI_Cut(face1, face2);

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




			//OA Handle(AIS_LengthDimension) LenDim1 = new AIS_LengthDimension(Point3, Point4, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) anAspectDim1 = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString aValue = LenDim1->GetValue();
			//OA TCollection_AsciiString atext = "DIM1 = ";
			//OA anAspectDim1->SetValueStringFormat(atext + aValue);
			//OA anAspectDim1->MakeArrows3d(Standard_False);
			//OA anAspectDim1->MakeText3d(Standard_False);
			//OA anAspectDim1->TextAspect()->SetHeight(0.3);
			//OA anAspectDim1->ArrowAspect()->SetLength(0.2);
			//OA anAspectDim1->MakeTextShaded(Standard_True);
			//OA anAspectDim1->SetCommonColor(Quantity_NOC_BLACK);
			//OA anAspectDim1->MakeUnitsDisplayed(Standard_False);
			//OA anAspectDim1->SetArrowTailSize(0.3);
			//OA anAspectDim1->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA LenDim1->SetTextPosition(PointDIM1Pos);
			//OA LenDim1->SetDimensionAspect(anAspectDim1);
			//_geoWindow->getAisContext()->Display(LenDim1, true);
			//OA beamView::_nameAisPair.insert(name, LenDim1);
			//OA _LenDim1 = LenDim1;

			//OA Handle(AIS_LengthDimension) aLenDim2 = new AIS_LengthDimension(Point4, Point1, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) anAspectDim2 = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t2Value = aLenDim2->GetValue();
			//OA TCollection_AsciiString t2text = "DIM2 = ";
			//OA anAspectDim2->SetValueStringFormat(t2text + t2Value);
			//OA anAspectDim2->MakeArrows3d(Standard_False);
			//OA anAspectDim2->MakeText3d(Standard_False);
			//OA anAspectDim2->TextAspect()->SetHeight(0.3);
			//OA anAspectDim2->ArrowAspect()->SetLength(0.2);
			//OA anAspectDim2->MakeTextShaded(Standard_True);
			//OA anAspectDim2->SetCommonColor(Quantity_NOC_BLACK);
			//OA anAspectDim2->MakeUnitsDisplayed(Standard_False);
			//OA anAspectDim2->SetArrowTailSize(0.3);
			//OA anAspectDim2->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA aLenDim2->SetTextPosition(PointDIM2Pos);
			//OA aLenDim2->SetDimensionAspect(anAspectDim2);
			//_geoWindow->getAisContext()->Display(aLenDim2, true);
			//OA beamView::_nameAisPair.insert(name, aLenDim2);
			//OA _aLenDim2 = aLenDim2;


			//OA Handle(AIS_LengthDimension) aLenDim3 = new AIS_LengthDimension(Edge1, Edge5, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) anAspectDim3 = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t1Value = aLenDim3->GetValue();
			//OA TCollection_AsciiString t1text = "DIM3 = ";
			//OA anAspectDim3->SetValueStringFormat(t1text + t1Value);
			//OA anAspectDim3->MakeArrows3d(Standard_False);
			//OA anAspectDim3->MakeText3d(Standard_False);
			//OA anAspectDim3->TextAspect()->SetHeight(0.3);
			//OA anAspectDim3->ArrowAspect()->SetLength(0.2);
			//OA anAspectDim3->MakeTextShaded(Standard_True);
			//OA anAspectDim3->SetCommonColor(Quantity_NOC_BLACK);
			//OA anAspectDim3->MakeUnitsDisplayed(Standard_False);
			//OA anAspectDim3->SetArrowTailSize(0.3);
			//OA anAspectDim3->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA aLenDim3->SetTextPosition(PointDIM3Pos);
			//OA aLenDim3->SetDimensionAspect(anAspectDim3);
			//_geoWindow->getAisContext()->Display(aLenDim3, true);
			//OA beamView::_nameAisPair.insert(name, aLenDim3);
			//OA _aLenDim3 = aLenDim3;

			//OA Handle(AIS_LengthDimension) LenDim4 = new AIS_LengthDimension(Edge3, Edge7, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) anAspectDim4 = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValue = LenDim4->GetValue();
			//OA TCollection_AsciiString btext = "DIM4 = ";
			//OA anAspectDim4->SetValueStringFormat(btext + bValue);
			//OA anAspectDim4->MakeArrows3d(Standard_False);
			//OA anAspectDim4->MakeText3d(Standard_False);
			//OA anAspectDim4->TextAspect()->SetHeight(0.3);
			//OA anAspectDim4->ArrowAspect()->SetLength(0.2);
			//OA anAspectDim4->MakeTextShaded(Standard_True);
			//OA anAspectDim4->SetCommonColor(Quantity_NOC_BLACK);
			//OA anAspectDim4->MakeUnitsDisplayed(Standard_False);
			//OA anAspectDim4->SetArrowTailSize(0.3);
			//OA anAspectDim4->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA LenDim4->SetTextPosition(PointDIM4Pos);
			//OA LenDim4->SetDimensionAspect(anAspectDim4);
			//_geoWindow->getAisContext()->Display(LenDim4, true);
			//OA beamView::_nameAisPair.insert(name, LenDim4);
			//OA _LenDim4 = LenDim4;

			//OA Handle(AIS_LengthDimension) LenDim5 = new AIS_LengthDimension(Edge2, Edge6, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) anAspectDim5 = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValueDIM5 = LenDim5->GetValue();
			//OA TCollection_AsciiString btextDim5 = "DIM5 = ";
			//OA anAspectDim5->SetValueStringFormat(btextDim5 + bValueDIM5);
			//OA anAspectDim5->MakeArrows3d(Standard_False);
			//OA anAspectDim5->MakeText3d(Standard_False);
			//OA anAspectDim5->TextAspect()->SetHeight(0.3);
			//OA anAspectDim5->ArrowAspect()->SetLength(0.2);
			//OA anAspectDim5->MakeTextShaded(Standard_True);
			//OA anAspectDim5->SetCommonColor(Quantity_NOC_BLACK);
			//OA anAspectDim5->MakeUnitsDisplayed(Standard_False);
			//OA anAspectDim5->SetArrowTailSize(0.3);
			//OA anAspectDim5->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA LenDim5->SetTextPosition(PointDIM5Pos);
			//OA LenDim5->SetDimensionAspect(anAspectDim5);
			//_geoWindow->getAisContext()->Display(LenDim5, true);
			//OA beamView::_nameAisPair.insert(name, LenDim5);
			//OA _LenDim5 = LenDim5;

			//OA Handle(AIS_LengthDimension) LenDim6 = new AIS_LengthDimension(Edge4, Edge8, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) anAspectDim6 = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValueDim6 = LenDim6->GetValue();
			//OA TCollection_AsciiString btextDim6 = "DIM6 = ";
			//OA anAspectDim6->SetValueStringFormat(btextDim6 + bValueDim6);
			//OA anAspectDim6->MakeArrows3d(Standard_False);
			//OA anAspectDim6->MakeText3d(Standard_False);
			//OA anAspectDim6->TextAspect()->SetHeight(0.3);
			//OA anAspectDim6->ArrowAspect()->SetLength(0.2);
			//OA anAspectDim6->MakeTextShaded(Standard_True);
			//OA anAspectDim6->SetCommonColor(Quantity_NOC_BLACK);
			//OA anAspectDim6->MakeUnitsDisplayed(Standard_False);
			//OA anAspectDim6->SetArrowTailSize(0.3);
			//OA anAspectDim6->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA LenDim6->SetTextPosition(PointDIM6Pos);
			//OA LenDim6->SetDimensionAspect(anAspectDim6);
			//_geoWindow->getAisContext()->Display(LenDim6, true);
			//OA beamView::_nameAisPair.insert(name, LenDim6);
			//OA _LenDim6 = LenDim6;


			double H = DIM2;
			double W = DIM1;
			double TL = DIM6;
			double TR = DIM5;
			double TB = DIM4;
			double TT = DIM3;
			double wi = W - TL - TR;
			double hi = H - TT - TB;
			//截面参数
			area = H * (TL + TR) + (W - TL - TR) * (TT + TB);
			double Zc = ((0.5 * W - 0.5 * TR) * TR * (H - TT - TB) - (0.5 * W - 0.5 * TL) * ((H - TT - TB) * TL)) / area;
			double Yc = ((0.5 * H - 0.5 * TT) * ((W - TL - TR) * TT) - (0.5 * H - 0.5 * TB) * (W - TL - TR) * TB) / area;
			//积分的上下界
			double B = 0.5 * H - Yc;
			double A = -0.5 * H - Yc;
			double b = 0.5 * H - TT - Yc;
			double a = TB - 0.5 * H - Yc;
			double D = 0.5 * W - Zc;
			double C = -0.5 * W - Zc;
			double d = 0.5 * W - TR - Zc;
			double c = TL - 0.5 * W - Zc;
			//Iz和Iy
			Iz = W / 3.0 * (pow(B, 3) - pow(A, 3)) - wi / 3.0 * (pow(b, 3) - pow(a, 3));
			Iy = H / 3.0 * (pow(D, 3) - pow(C, 3)) - hi / 3.0 * (pow(d, 3) - pow(c, 3));
			//Iyz
			Iyz = (B * B - b * b) * (d * d - c * c) / 4.0 + (a * a - A * A) * (d * d - c * c) / 4.0 +
				(c * c - C * C) * (B * B - A * A) / 4.0 + (D * D - d * d) * (B * B - A * A) / 4.0;
			//自由扭转常数
			double ASTAR = (W - 0.5 * (TL + TR)) * (H - 0.5 * (TT + TB));
			double DEN = (H - 0.5 * (TT + TB)) * (1.0 / TL + 1.0 / TR) + (W - 0.5 * (TL + TR)) * (1.0 / TT + 1.0 / TB);
			J = 4.0 * ASTAR * ASTAR / DEN;

			//有效剪切面积
			K1 = (H - TT - TB) * (TL + TR) / area;
			K2 = (W - TL - TR) * (TT + TB) / area;

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
}

