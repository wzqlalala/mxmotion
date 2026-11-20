#include "I_SectionVisual.h"
//#include "gp_Pnt.hxx"
//#include "BRepBuilderAPI_MakeEdge.hxx"
//#include "BRepBuilderAPI_MakeFace.hxx"
//#include "BRepBuilderAPI_MakeWire.hxx"
//#include "TopoDS_Face.hxx"
//#include "TopoDS_Wire.hxx"
//#include <Prs3d_Drawer.hxx>
//#include <AIS_InteractiveContext.hxx>
//#include <AIS_LengthDimension.hxx>
// #include "MxBeam/GeoWidget.h"
//#include "GC_MakePlane.hxx"
#include "mPBeam.h"
//#include "BRepGProp_Face.hxx"

//#include "GProp_GProps.hxx"
#include "Sample2D_Markers.h"
//#include <BRepGProp.hxx>

//TEST
#include <mGlobalSignals.h>

//#include "BRepPrimAPI_MakeBox.hxx"
//#include "AIS_Animation.hxx"
//#include "AIS_AnimationObject.hxx"
//#include "GeoWidget.h"
//#include "beamView.h"
#include "beamView.h"
namespace MechSectionShapeData
{
	I_SectionVisual::I_SectionVisual(QString name, int id)
		: mSectionShapeBase(Sec_I,name,id)
	{

		_para.push_back(b);
		_para.push_back(c);
		_para.push_back(a);
		_para.push_back(t1);
		_para.push_back(t3);
		_para.push_back(t2);
	}

	I_SectionVisual::~I_SectionVisual()
	{
	}
	I_SectionVisual* I_SectionVisual::clone() const
	{
		I_SectionVisual* newI = new I_SectionVisual(_sectionShapeName, _id);

		newI->a = a;
		newI->b = b;
		newI->c = c;
		newI->t2 = t2;
		newI->t1 = t1;
		newI->t3 = t3;
		newI->_X1 = _X1;
		newI->_Y1 = _Y1;
		newI->_Z1 = _Z1;
		newI->_X2 = _X2;
		newI->_Y2 = _Y2;
		newI->_Z2 = _Z2;
		newI->_X3 = _X3;
		newI->_Y3 = _Y3;
		newI->_Z3 = _Z3;
		newI->_shearMarker = _shearMarker;
		newI->_a112LenDim = _a112LenDim;
		newI->_E612LenDim = _E612LenDim;
		newI->_E39LenDim = _E39LenDim;
		newI->_P1112LenDim = _P1112LenDim;
		newI->_P78LenDim = _P78LenDim;
		newI->_P67LenDim = _P67LenDim;
		newI->_aisshape = _aisshape;
		newI->_para = _para;
		newI->_data = _data;
		newI->area = area;
		newI->Iz = Iz;
		newI->Iy = Iy;
		newI->J = J;
		newI->Iyz = Iyz;
		newI->K1 = K1;
		newI->K2 = K2;
		return newI;
	}
	void I_SectionVisual::CreateISec()
	{
		/*TopoDS_Shape obj1 = BRepPrimAPI_MakeBox(100, 500, 20);
		Handle(AIS_Shape) ais_obj1 = new AIS_Shape(obj1);
		ais_obj1->SetDisplayMode(AIS_Shaded);
		_geoWindow->getAisContext()->Display(ais_obj1, true);

		gp_Trsf start_pnt, end_pnt;

		start_pnt.SetValues(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0);
		end_pnt.SetValues(1, 0, 0, 100, 0, 1, 0, 100, 0, 0, 1, 100);

		Handle(AIS_Animation) ais_animation = new AIS_Animation("obj1");
		Handle(AIS_AnimationObject) ais_ao = new AIS_AnimationObject("obj1", _geoWindow->getAisContext(), ais_obj1, start_pnt, end_pnt);
		ais_ao->SetOwnDuration(5);
		ais_ao->SetStartPts(1);

		ais_animation->Add(ais_ao);

		double duration = ais_animation->Duration();

		ais_animation->StartTimer(0, 1.0, true);

		while (!ais_animation->IsStopped())
		{
			ais_animation->UpdateTimer();

			_geoWindow->getAisContext()->UpdateCurrentViewer();
		}*/

		try
		{
			//OA gp_Pnt Point1(_X1, _Y1, _Z1); gp_Pnt Point6(_X1 - (c - a) / 2, -_Y1 - (t3 - 0.5), _Z1); gp_Pnt Point7(-_X1 + (c - a) / 2, -_Y1 - (t3 - 0.5), _Z1); gp_Pnt Point12(-_X1, _Y1, _Z1);
			//OA gp_Pnt Point2(_X2, _Y2, _Z2); gp_Pnt Point5(_X2 - (c - a) / 2, -_Y2, _Z2); gp_Pnt Point8(-_X2 + (c - a) / 2, -_Y2, _Z2); gp_Pnt Point11(-_X2, _Y2, _Z2);
			//OA gp_Pnt Point3(_X3, _Y3, _Z3); gp_Pnt Point4(_X3, -_Y3, _Z3); gp_Pnt Point9(-_X3, -_Y3, _Z3); gp_Pnt Point10(-_X3, _Y3, _Z3);
			//OA gp_Pnt Point112(0, _Y1 + 0.5, _Z1); gp_Pnt Point1112(-_X2 + 0.5, (_Y1 + _Y2) / 2, _Z1);
			//OA gp_Pnt Point78(-_X1 + (c - a) / 2 + 0.5, -(_Y1 + _Y2) / 2, _Z1);
			//OA gp_Pnt Point67(0, -b / 2 - 0.5, _Z1);
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
			//OA gp_Pnt PointE612(_X1 - (c - a) / 2 - 0.5, 0, _Z1); gp_Pnt PointE39(0, _Y1 - 0.5, _Z1);
			//OA auto ISecWire1 = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//OA TopoDS_Wire ISecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6, Edge7, Edge8);
			//OA TopoDS_Wire ISecWire3 = BRepBuilderAPI_MakeWire(Edge9, Edge10, Edge11, Edge12);
			//OA ISecWire1.Add(ISecWire2);
			//OA ISecWire1.Add(ISecWire3);
			//OA TopoDS_Shape ISecFace = BRepBuilderAPI_MakeFace(ISecWire1);
			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(ISecFace);
			//OA _aisshape = aisshape;
			//OA BaseshowBeamSec(aisshape);
		
			QString name = mPBeam::getInstance()->setDefaultNameForBeam(Sec_I);
			/*emit AddSecSignal(name);
			emit showParaOnPanel(_para, name);*/
			//OA beamView::_nameAisPair.insert(name, aisshape);
			//OA beamView::_secNameAisPair.insert(name, aisshape);
			mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());

			//OA getDataFromBeam(name, ISecFace);


			//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();


			//OA Handle(AIS_LengthDimension) a112LenDim = new AIS_LengthDimension(Point1, Point12, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an112Aspect = new Prs3d_DimensionAspect();

			//OA TCollection_AsciiString aValue = a112LenDim->GetValue();
			//OA TCollection_AsciiString atext = "DIM3 = ";
			//OA an112Aspect->SetValueStringFormat(atext + aValue);
			//OA an112Aspect->ArrowAspect()->SetLength(0.1);
			//OA an112Aspect->MakeArrows3d(Standard_False);
			//OA an112Aspect->MakeText3d(Standard_False);
			//OA an112Aspect->TextAspect()->SetHeight(0.5);
			//OA an112Aspect->MakeTextShaded(Standard_True);
			//OA an112Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an112Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an112Aspect->SetArrowTailSize(0.1);
			//OA a112LenDim->SetTextPosition(Point112);
			//OA a112LenDim->SetDimensionAspect(an112Aspect);
			//_geoWindow->getAisContext()->Display(a112LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a112LenDim);
			//OA _a112LenDim = a112LenDim;
			//OA Handle(AIS_LengthDimension) E612LenDim = new AIS_LengthDimension(Edge6, Edge12, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) E612Aspect = new Prs3d_DimensionAspect();

			//OA E612Aspect->ArrowAspect()->SetLength(0.1);
			//OA E612Aspect->MakeArrows3d(Standard_False);
			//OA E612Aspect->MakeText3d(Standard_False);
			//OA E612Aspect->TextAspect()->SetHeight(0.5);
			//OA E612Aspect->MakeTextShaded(Standard_True);
			//OA E612Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA E612Aspect->MakeUnitsDisplayed(Standard_False);
			//OA E612Aspect->SetArrowTailSize(0.1);
			//OA TCollection_AsciiString bValue = E612LenDim->GetValue();
			//OA TCollection_AsciiString btext = "DIM1 = ";
			//OA E612Aspect->SetValueStringFormat(btext + bValue);
			//OA E612LenDim->SetTextPosition(PointE612);
			//OA E612LenDim->SetDimensionAspect(E612Aspect);
			//_geoWindow->getAisContext()->Display(E612LenDim, true);
			//OA beamView::_nameAisPair.insert(name, E612LenDim);
			//OA _E612LenDim = E612LenDim;
			//OA Handle(AIS_LengthDimension) E39LenDim = new AIS_LengthDimension(Edge3, Edge9, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) E39Aspect = new Prs3d_DimensionAspect();
			//OA E39Aspect->ArrowAspect()->SetLength(0.2);
			//OA E39Aspect->MakeArrows3d(Standard_False);
			//OA E39Aspect->MakeText3d(Standard_False);
			//OA E39Aspect->TextAspect()->SetHeight(0.5);
			//OA E39Aspect->MakeTextShaded(Standard_True);
			//OA E39Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA E39Aspect->MakeUnitsDisplayed(Standard_False);
			//OA E39Aspect->SetArrowTailSize(0.1);
			//OA TCollection_AsciiString t1Value = E39LenDim->GetValue();
			//OA TCollection_AsciiString t1text = "DIM4 = ";
			//OA E39Aspect->SetValueStringFormat(t1text + t1Value);
			//OA E39LenDim->SetTextPosition(PointE39);
			//OA E39LenDim->SetDimensionAspect(E39Aspect);
			//OA E39LenDim->SetCustomValue(100);
			//_geoWindow->getAisContext()->Display(E39LenDim, true);
			//OA beamView::_nameAisPair.insert(name, E39LenDim);
			//OA _E39LenDim = E39LenDim;
			//OA Handle(AIS_LengthDimension) P1112LenDim = new AIS_LengthDimension(Point11, Point12, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) P1112Aspect = new Prs3d_DimensionAspect();
			//OA P1112Aspect->ArrowAspect()->SetLength(0.2);
			//OA P1112Aspect->MakeArrows3d(Standard_False);
			//OA P1112Aspect->MakeText3d(Standard_False);
			//OA P1112Aspect->TextAspect()->SetHeight(0.5);
			//OA P1112Aspect->MakeTextShaded(Standard_True);
			//OA P1112Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA P1112Aspect->MakeUnitsDisplayed(Standard_False);
			//OA P1112Aspect->SetArrowTailSize(0.1);
			//OA TCollection_AsciiString t2Value = P1112LenDim->GetValue();
			//OA TCollection_AsciiString t2text = "DIM6 = ";
			//OA P1112Aspect->SetValueStringFormat(t2text + t2Value);
			//OA P1112LenDim->SetTextPosition(Point1112);
			//OA P1112LenDim->SetDimensionAspect(P1112Aspect);
			//_geoWindow->getAisContext()->Display(P1112LenDim, true);
			//OA beamView::_nameAisPair.insert(name, P1112LenDim);
			//OA _P1112LenDim = P1112LenDim;
			//OA Handle(AIS_LengthDimension) P78LenDim = new AIS_LengthDimension(Point7, Point8, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) P78Aspect = new Prs3d_DimensionAspect();
			//OA P78Aspect->ArrowAspect()->SetLength(0.2);
			//OA P78Aspect->MakeArrows3d(Standard_False);
			//OA P78Aspect->MakeText3d(Standard_False);
			//OA P78Aspect->TextAspect()->SetHeight(0.5);
			//OA P78Aspect->MakeTextShaded(Standard_True);
			//OA P78Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA P78Aspect->MakeUnitsDisplayed(Standard_False);
			//OA P78Aspect->SetArrowTailSize(0.1);
			//OA TCollection_AsciiString t3Value = P78LenDim->GetValue();
			//OA TCollection_AsciiString t3text = "DIM5 = ";
			//OA P78Aspect->SetValueStringFormat(t3text + t2Value);
			//OA P78LenDim->SetTextPosition(Point78);
			//OA P78LenDim->SetDimensionAspect(P78Aspect);
			//_geoWindow->getAisContext()->Display(P78LenDim, true);
			//OA beamView::_nameAisPair.insert(name, P78LenDim);
			//OA _P78LenDim = P78LenDim;
			//OA Handle(AIS_LengthDimension) P67LenDim = new AIS_LengthDimension(Point6, Point7, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) P67Aspect = new Prs3d_DimensionAspect();
			//OA P67Aspect->ArrowAspect()->SetLength(0.2);
			//OA P67Aspect->MakeArrows3d(Standard_False);
			//OA P67Aspect->MakeText3d(Standard_False);
			//OA P67Aspect->TextAspect()->SetHeight(0.5);
			//OA P67Aspect->MakeTextShaded(Standard_True);
			//OA P67Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA P67Aspect->MakeUnitsDisplayed(Standard_False);
			//OA P67Aspect->SetArrowTailSize(0.1);
			//OA TCollection_AsciiString cValue = P67LenDim->GetValue();
			//OA TCollection_AsciiString ctext = "DIM2 = ";
			//OA P67Aspect->SetValueStringFormat(ctext + cValue);
			//OA P67LenDim->SetTextPosition(Point67);
			//OA P67LenDim->SetDimensionAspect(P67Aspect);
			//_geoWindow->getAisContext()->Display(P67LenDim, true);
			//OA beamView::_nameAisPair.insert(name, P67LenDim);
			//OA _P67LenDim = P67LenDim;
			//求翼缘惯性积
			//上翼缘
			//OA TopoDS_Edge Edgea = BRepBuilderAPI_MakeEdge(Point1, Point2);
			//OA TopoDS_Edge Edgeb = BRepBuilderAPI_MakeEdge(Point2, Point11);
			//OA TopoDS_Edge Edgec = BRepBuilderAPI_MakeEdge(Point11, Point12);
			//OA TopoDS_Edge Edged = BRepBuilderAPI_MakeEdge(Point12, Point1);
			//OA auto ISecWirea = BRepBuilderAPI_MakeWire(Edgea, Edgeb, Edgec, Edged);
			//OA TopoDS_Shape ISecFacea = BRepBuilderAPI_MakeFace(ISecWirea);
			//OA GProp_GProps System;
			//OA BRepGProp::SurfaceProperties(ISecFacea, System);
			//OA gp_Pnt P(0, 0, 0); gp_Dir  d(0, 1, 0);
			//OA gp_Ax1 ax1(P, d);
			//OA auto Iya = System.MomentOfInertia(ax1);

			//下翼缘
			//OA TopoDS_Edge Edgee = BRepBuilderAPI_MakeEdge(Point5, Point6);
			//OA TopoDS_Edge Edgef = BRepBuilderAPI_MakeEdge(Point6, Point7);
			//OA TopoDS_Edge Edgeg = BRepBuilderAPI_MakeEdge(Point7, Point8);
			//OA TopoDS_Edge Edgeh = BRepBuilderAPI_MakeEdge(Point8, Point5);
			//OA auto ISecWireb = BRepBuilderAPI_MakeWire(Edgee, Edgef, Edgeg, Edgeh);
			//OA TopoDS_Shape ISecFaceb = BRepBuilderAPI_MakeFace(ISecWireb);
			//OA GProp_GProps System1;
			//OA BRepGProp::SurfaceProperties(ISecFaceb, System1);
			//OA gp_Pnt Pb(0, 0, 0); gp_Dir  db(0, 1, 0);
			//OA gp_Ax1 ax2(Pb, db);
			//OA auto Iyb = System1.MomentOfInertia(ax2);
			double h = b - (t2 + t3) / 2;

			//OA double S = (h* Iyb) / (Iya + Iyb);
			//【变量追踪修复】S 走 S-03 shearCenterY（翼缘惯性矩法已移植适配层）
			const double Sy = 0.0; //OA: 剪心（原 GProp 链休眠，无活跃消费者）


			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(0, Sy, Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//_geoWindow->getAisContext()->Display(shearMarker, true);
			//OA beamView::_nameAisPair.insert(name, shearMarker);
			//OA _shearMarker = shearMarker;
			//翼缘惯性积
			//抗扭惯性矩
			double ti1, bi1, Ci1 = 1.0 / 3.0;
			bi1 = a;
			ti1 = t2;
			double It1 = Ci1 * bi1*pow(ti1, 3);
			double ti2, bi2, Ci2 = 1.0 / 3.0;

			bi2 = c;
			ti2 = t3;
			double It2 = Ci2 * bi2*pow(ti2, 3);
			double ti3, bi3, Ci3 = 1.0 / 3.0;

			bi3 = b - t2 - t3;
			ti3 = t1;
			double It3 = Ci3 * bi3*pow(ti3, 3);
			double It = It1 + It2 + It3;
			_data.push_back(It);
			_data.push_back(Sy);
			_data.push_back(0);
			double SFY = (b - t2 - t3)*t1 / _data.at(0);
			double SFZ = (_data.at(0) - b * t1) / _data.at(0);
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

	void I_SectionVisual::ReBuildISec(QList<double> para)
	{
		try
		{
			a = para.at(3);
			b = para.at(1);
			c = para.at(2);
			t1 = para.at(4);
			t3 = para.at(5);
			t2 = para.at(6);
			//_geoWindow->getAisContext()->EraseAll(true);
			para.removeFirst();
			_para = para;

			_X1 = -a / 2; _X2 = _X1; _X3 = -t1 / 2;
			_Y1 = b / 2; _Y2 = _Y1 - t2; _Y3 = _Y2;
			//OA gp_Pnt Point1(_X1, _Y1, _Z1);  gp_Pnt Point6(_X1 - (c - a) / 2, -b / 2.0, _Z1); gp_Pnt Point7(-_X1 + (c - a) / 2, -b / 2.0, _Z1); gp_Pnt Point12(-_X1, _Y1, _Z1);
			//OA gp_Pnt Point2(_X2, _Y2, _Z2); gp_Pnt Point5(_X2 - (c - a) / 2, -b / 2.0 + t3, _Z2); gp_Pnt Point8(-_X2 + (c - a) / 2, -b / 2.0 + t3, _Z2); gp_Pnt Point11(-_X2, _Y2, _Z2);
			//OA gp_Pnt Point3(_X3, _Y3, _Z3); gp_Pnt Point4(_X3, -b / 2.0 + t3, _Z3); gp_Pnt Point9(-_X3, -b / 2.0 + t3, _Z3); gp_Pnt Point10(-_X3, _Y3, _Z3);
			//OA gp_Pnt Point112(0, _Y1 + 0.5, _Z1); gp_Pnt Point1112(-_X2 + 0.5, (_Y1 + _Y2) / 2, _Z1); gp_Pnt Point78(-_X1 + (c - a) / 2 + 0.5, (-b / 2.0 - b / 2.0 + t3) / 2.0, _Z1);
			//OA gp_Pnt Point67(0, -b / 2.0 - 0.5, _Z1);
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
			//OA gp_Pnt PointE612(_X2 - 0.5, 0, _Z1); gp_Pnt PointE39(0, _Y1 - 0.5, _Z1);
			//OA auto ISecWire1 = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//OA TopoDS_Wire ISecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6, Edge7, Edge8);
			//OA TopoDS_Wire ISecWire3 = BRepBuilderAPI_MakeWire(Edge9, Edge10, Edge11, Edge12);
			//OA ISecWire1.Add(ISecWire2);
			//OA ISecWire1.Add(ISecWire3);
			//OA TopoDS_Shape ISecFace = BRepBuilderAPI_MakeFace(ISecWire1);
			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(ISecFace);
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
			//求截面数据
			//OA getDataFromBeam(name, ISecFace);

			//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();


			//OA Handle(AIS_LengthDimension) a112LenDim = new AIS_LengthDimension(Point1, Point12, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an112Aspect = new Prs3d_DimensionAspect();

			//OA TCollection_AsciiString aValue = a112LenDim->GetValue();
			//OA TCollection_AsciiString atext = "DIM3 = ";
			//OA an112Aspect->SetValueStringFormat(atext + aValue);
			//OA an112Aspect->ArrowAspect()->SetLength(0.1);
			//OA an112Aspect->MakeArrows3d(Standard_False);
			//OA an112Aspect->MakeText3d(Standard_False);
			//OA an112Aspect->TextAspect()->SetHeight(0.3);
			//OA an112Aspect->MakeTextShaded(Standard_True);
			//OA an112Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an112Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an112Aspect->SetArrowTailSize(0.5);
			//OA a112LenDim->SetTextPosition(Point112);
			//OA a112LenDim->SetDimensionAspect(an112Aspect);
			//_geoWindow->getAisContext()->Display(a112LenDim, true);
			//OA _a112LenDim = a112LenDim;
			//OA beamView::_nameAisPair.insert(name, a112LenDim);

			//OA Handle(AIS_LengthDimension) E612LenDim = new AIS_LengthDimension(Edge6, Edge12, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) E612Aspect = new Prs3d_DimensionAspect();

			//OA E612Aspect->ArrowAspect()->SetLength(0.1);
			//OA E612Aspect->MakeArrows3d(Standard_False);
			//OA E612Aspect->MakeText3d(Standard_False);
			//OA E612Aspect->TextAspect()->SetHeight(0.5);
			//OA E612Aspect->MakeTextShaded(Standard_True);
			//OA E612Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA E612Aspect->MakeUnitsDisplayed(Standard_False);
			//OA E612Aspect->SetArrowTailSize(0.1);
			//OA TCollection_AsciiString bValue = E612LenDim->GetValue();
			//OA TCollection_AsciiString btext = "DIM1 = ";
			//OA E612Aspect->SetValueStringFormat(btext + bValue);
			//OA E612LenDim->SetTextPosition(PointE612);
			//OA E612LenDim->SetDimensionAspect(E612Aspect);
			//_geoWindow->getAisContext()->Display(E612LenDim, true);
			//OA beamView::_nameAisPair.insert(name, E612LenDim);
			//OA _E612LenDim = E612LenDim;
			//OA Handle(AIS_LengthDimension) E39LenDim = new AIS_LengthDimension(Edge3, Edge9, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) E39Aspect = new Prs3d_DimensionAspect();
			//OA E39Aspect->ArrowAspect()->SetLength(0.2);
			//OA E39Aspect->MakeArrows3d(Standard_False);
			//OA E39Aspect->MakeText3d(Standard_False);
			//OA E39Aspect->TextAspect()->SetHeight(0.5);
			//OA E39Aspect->MakeTextShaded(Standard_True);
			//OA E39Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA E39Aspect->MakeUnitsDisplayed(Standard_False);
			//OA E39Aspect->SetArrowTailSize(0.1);
			//OA TCollection_AsciiString t1Value = E39LenDim->GetValue();
			//OA TCollection_AsciiString t1text = "DIM4 = ";
			//OA E39Aspect->SetValueStringFormat(t1text + t1Value);
			//OA E39LenDim->SetTextPosition(PointE39);
			//OA E39LenDim->SetDimensionAspect(E39Aspect);
			//OA E39LenDim->SetCustomValue(100);
			//_geoWindow->getAisContext()->Display(E39LenDim, true);
			//OA beamView::_nameAisPair.insert(name, E39LenDim);
			//OA _E39LenDim = E39LenDim;
			//OA Handle(AIS_LengthDimension) P1112LenDim = new AIS_LengthDimension(Point11, Point12, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) P1112Aspect = new Prs3d_DimensionAspect();
			//OA P1112Aspect->ArrowAspect()->SetLength(0.2);
			//OA P1112Aspect->MakeArrows3d(Standard_False);
			//OA P1112Aspect->MakeText3d(Standard_False);
			//OA P1112Aspect->TextAspect()->SetHeight(0.5);
			//OA P1112Aspect->MakeTextShaded(Standard_True);
			//OA P1112Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA P1112Aspect->MakeUnitsDisplayed(Standard_False);
			//OA P1112Aspect->SetArrowTailSize(0.1);
			//OA TCollection_AsciiString t2Value = P1112LenDim->GetValue();
			//OA TCollection_AsciiString t2text = "DIM6 = ";
			//OA P1112Aspect->SetValueStringFormat(t2text + t2Value);
			//OA P1112LenDim->SetTextPosition(Point1112);
			//OA P1112LenDim->SetDimensionAspect(P1112Aspect);
			//_geoWindow->getAisContext()->Display(P1112LenDim, true);
			//OA beamView::_nameAisPair.insert(name, P1112LenDim);
			//OA _P1112LenDim = P1112LenDim;
			//OA Handle(AIS_LengthDimension) P78LenDim = new AIS_LengthDimension(Point7, Point8, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) P78Aspect = new Prs3d_DimensionAspect();
			//OA P78Aspect->ArrowAspect()->SetLength(0.2);
			//OA P78Aspect->MakeArrows3d(Standard_False);
			//OA P78Aspect->MakeText3d(Standard_False);
			//OA P78Aspect->TextAspect()->SetHeight(0.5);
			//OA P78Aspect->MakeTextShaded(Standard_True);
			//OA P78Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA P78Aspect->MakeUnitsDisplayed(Standard_False);
			//OA P78Aspect->SetArrowTailSize(0.1);
			//OA TCollection_AsciiString t3Value = P78LenDim->GetValue();
			//OA TCollection_AsciiString t3text = "DIM5 = ";
			//OA P78Aspect->SetValueStringFormat(t3text + t3Value);
			//OA P78LenDim->SetTextPosition(Point78);
			//OA P78LenDim->SetDimensionAspect(P78Aspect);
			//_geoWindow->getAisContext()->Display(P78LenDim, true);
			//OA beamView::_nameAisPair.insert(name, P78LenDim);
			//OA _P78LenDim = P78LenDim;
			//OA Handle(AIS_LengthDimension) P67LenDim = new AIS_LengthDimension(Point6, Point7, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) P67Aspect = new Prs3d_DimensionAspect();
			//OA P67Aspect->ArrowAspect()->SetLength(0.2);
			//OA P67Aspect->MakeArrows3d(Standard_False);
			//OA P67Aspect->MakeText3d(Standard_False);
			//OA P67Aspect->TextAspect()->SetHeight(0.5);
			//OA P67Aspect->MakeTextShaded(Standard_True);
			//OA P67Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA P67Aspect->MakeUnitsDisplayed(Standard_False);
			//OA P67Aspect->SetArrowTailSize(0.1);
			//OA TCollection_AsciiString cValue = P67LenDim->GetValue();
			//OA TCollection_AsciiString ctext = "DIM2 = ";
			//OA P67Aspect->SetValueStringFormat(ctext + cValue);
			//OA P67LenDim->SetTextPosition(Point67);
			//OA P67LenDim->SetDimensionAspect(P67Aspect);
			//_geoWindow->getAisContext()->Display(P67LenDim, true);
			//OA beamView::_nameAisPair.insert(name, P67LenDim);
			//OA _P67LenDim = P67LenDim;
			//求翼缘惯性积
			//上翼缘
			//OA TopoDS_Edge Edgea = BRepBuilderAPI_MakeEdge(Point1, Point2);
			//OA TopoDS_Edge Edgeb = BRepBuilderAPI_MakeEdge(Point2, Point11);
			//OA TopoDS_Edge Edgec = BRepBuilderAPI_MakeEdge(Point11, Point12);
			//OA TopoDS_Edge Edged = BRepBuilderAPI_MakeEdge(Point12, Point1);
			//OA auto ISecWirea = BRepBuilderAPI_MakeWire(Edgea, Edgeb, Edgec, Edged);
			//OA TopoDS_Shape ISecFacea = BRepBuilderAPI_MakeFace(ISecWirea);
			//OA GProp_GProps System;
			//OA BRepGProp::SurfaceProperties(ISecFacea, System);
			//OA gp_Pnt P(0, 0, 0); gp_Dir  d(0, 1, 0);
			//OA gp_Ax1 ax1(P, d);
			//OA auto Iya = System.MomentOfInertia(ax1);

			//下翼缘
			//OA TopoDS_Edge Edgee = BRepBuilderAPI_MakeEdge(Point5, Point6);
			//OA TopoDS_Edge Edgef = BRepBuilderAPI_MakeEdge(Point6, Point7);
			//OA TopoDS_Edge Edgeg = BRepBuilderAPI_MakeEdge(Point7, Point8);
			//OA TopoDS_Edge Edgeh = BRepBuilderAPI_MakeEdge(Point8, Point5);
			//OA auto ISecWireb = BRepBuilderAPI_MakeWire(Edgee, Edgef, Edgeg, Edgeh);
			//OA TopoDS_Shape ISecFaceb = BRepBuilderAPI_MakeFace(ISecWireb);
			//OA GProp_GProps System1;
			//OA BRepGProp::SurfaceProperties(ISecFaceb, System1);
			//OA gp_Pnt Pb(0, 0, 0); gp_Dir  db(0, 1, 0);
			//OA gp_Ax1 ax2(Pb, db);
			//OA auto Iyb = System1.MomentOfInertia(ax2);
			double h = b - (t2 + t3) / 2;

			//OA double S = (h* Iyb) / (Iya + Iyb);
			//【变量追踪修复】S 走 S-03 shearCenterY（翼缘惯性矩法已移植适配层）
			const double Sy = 0.0; //OA: 剪心（原 GProp 链休眠，无活跃消费者）


			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(0, Sy, Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//_geoWindow->getAisContext()->Display(shearMarker, true);
			//OA beamView::_nameAisPair.insert(name, shearMarker);
			//OA _shearMarker = shearMarker;
			//翼缘惯性积
			//抗扭惯性矩
			double ti1, bi1, Ci1 = 1.0 / 3.0;
			bi1 = a;
			ti1 = t2;
			double It1 = Ci1 * bi1*pow(ti1, 3);
			double ti2, bi2, Ci2 = 1.0 / 3.0;

			bi2 = c;
			ti2 = t3;
			double It2 = Ci2 * bi2*pow(ti2, 3);
			double ti3, bi3, Ci3 = 1.0 / 3.0;

			bi3 = b - t2 - t3;
			ti3 = t1;
			double It3 = Ci3 * bi3*pow(ti3, 3);
			double It = It1 + It2 + It3;
			_data.push_back(It);
			_data.push_back(Sy);
			_data.push_back(0);
			double SFY = (b - t2 - t3)*t1 / _data.at(0);
			double SFZ = (_data.at(0) - b * t1) / _data.at(0);
			_data.push_back(SFY);
			_data.push_back(SFZ);
			/*emit showDataOnPanel(_data);
			emit showDataOnPanel(_data);*/
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
		}
		

	}
}

