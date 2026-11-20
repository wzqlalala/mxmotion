#include "Channel2_SectionVisual.h"
//OA #include "gp_Pnt.hxx"
//OA #include "BRepBuilderAPI_MakeEdge.hxx"
//OA #include "BRepBuilderAPI_MakeFace.hxx"
//OA #include "BRepBuilderAPI_MakeWire.hxx"
//OA #include "TopoDS_Face.hxx"
//OA #include "TopoDS_Wire.hxx"
//OA #include <Prs3d_Drawer.hxx>
//OA #include <AIS_InteractiveContext.hxx>
//OA #include <AIS_LengthDimension.hxx>
// #include "MxBeam/GeoWidget.h"
//OA #include "GC_MakePlane.hxx"
#include "mPBeam.h"
//OA #include "BRepGProp_Face.hxx"

//OA #include <GProp_GProps.hxx>
//OA #include "Sample2D_Markers.h"
//OA #include <BRepGProp.hxx>
#include <mGlobalSignals.h>

//#include "GeoWidget.h"
#include "beamView.h"

namespace MechSectionShapeData
{
	Channel2_SectionVisual::Channel2_SectionVisual(QString name, int id)
		: MechSectionShapeData::mSectionShapeBase(Sec_Channel2,name,id)
	{

		_para.push_back(t1);
		_para.push_back(t2);
		_para.push_back(a);
		_para.push_back(b);
	
	}

	Channel2_SectionVisual::~Channel2_SectionVisual()
	{
	}
	Channel2_SectionVisual* Channel2_SectionVisual::clone() const
	{
		Channel2_SectionVisual* newSec = new Channel2_SectionVisual(_sectionShapeName, _id);

		newSec->a = a;
		newSec->b = b;
		newSec->t2 = t2;
		newSec->t1 = t1;
		newSec->_X = _X;
		newSec->_Y = _Y;
		newSec->_Z = _Z;


		newSec->_shearMarker = _shearMarker;
		newSec->_a18LenDim = _a18LenDim;
		newSec->_a12LenDim = _a12LenDim;
		newSec->_a23LenDim = _a23LenDim;
		newSec->_aE26LenDim = _aE26LenDim;
		newSec->_aisshape = _aisshape;
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
	void Channel2_SectionVisual::CreateChan2Sec()
	{
		try
		{
			//OA gp_Pnt Point1(_X, _Y, _Z); gp_Pnt Point2(_X, -_Y, _Z); gp_Pnt Point3(-_X, -_Y, _Z); gp_Pnt Point4(-_X, _Y, _Z);
			//OA gp_Pnt Point5(-_X - t1, _Y, _Z);	gp_Pnt Point8(_X + t1, _Y, _Z);
			//OA gp_Pnt Point6(-_X - t1, -_Y + t2, _Z); gp_Pnt Point7(_X + t1, -_Y + t2, _Z);
			//OA gp_Pnt Point18((2 * _X + t1) / 2, _Y + 0.5, _Z); gp_Pnt Point12(_X - 0.5, 0, 0); gp_Pnt Point23(0, -_Y - 0.5, 0);
			//OA gp_Pnt PointE26(-_X - t1, (-2 * _Y + t2) / 2, 0);
			//OA TopoDS_Edge Edge1 = BRepBuilderAPI_MakeEdge(Point1, Point2);
			//OA TopoDS_Edge Edge2 = BRepBuilderAPI_MakeEdge(Point2, Point3);
			//OA TopoDS_Edge Edge3 = BRepBuilderAPI_MakeEdge(Point3, Point4);
			//OA TopoDS_Edge Edge4 = BRepBuilderAPI_MakeEdge(Point4, Point5);
			//OA TopoDS_Edge Edge5 = BRepBuilderAPI_MakeEdge(Point5, Point6);
			//OA TopoDS_Edge Edge6 = BRepBuilderAPI_MakeEdge(Point6, Point7);
			//OA TopoDS_Edge Edge7 = BRepBuilderAPI_MakeEdge(Point7, Point8);
			//OA TopoDS_Edge Edge8 = BRepBuilderAPI_MakeEdge(Point8, Point1);
			//OA auto ChannelSecWire1 = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//OA TopoDS_Wire ChannelSecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6, Edge7, Edge8);
			//OA ChannelSecWire1.Add(ChannelSecWire2);
			//OA TopoDS_Shape ChannelSecFace = BRepBuilderAPI_MakeFace(ChannelSecWire1);
			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(ChannelSecFace);
			//OA _aisshape = aisshape;
			//OA BaseshowBeamSec(aisshape);
			QString name = mPBeam::getInstance()->setDefaultNameForBeam(Sec_Channel2);
	/*		emit AddSecSignal(name);
			emit showParaOnPanel(_para, name);*/
			//OA beamView::_nameAisPair.insert(name, aisshape);
			//OA beamView::_secNameAisPair.insert(name, aisshape);
			mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());
			//OA getDataFromBeam(name, ChannelSecFace);
			//剪切中心
			// //构建参考截面用来计算剪心
			//gp_Pnt Pointa1(0, -_Y, _Z), Pointa2(0, -_Y + t2, _Z);
			//TopoDS_Edge Edgea1 = BRepBuilderAPI_MakeEdge(Pointa1, Point3);
			//TopoDS_Edge Edgea2 = BRepBuilderAPI_MakeEdge(Point3, Point4);
			//TopoDS_Edge Edgea3 = BRepBuilderAPI_MakeEdge(Point4, Point5);
			//TopoDS_Edge Edgea4 = BRepBuilderAPI_MakeEdge(Point5, Point6);
			//TopoDS_Edge Edgea5 = BRepBuilderAPI_MakeEdge(Point6, Pointa2);
			//TopoDS_Edge Edgea6 = BRepBuilderAPI_MakeEdge(Pointa2, Pointa1);
			//auto Wire1 = BRepBuilderAPI_MakeWire(Edgea1, Edgea2, Edgea3, Edgea4);
			//TopoDS_Wire Wire2 = BRepBuilderAPI_MakeWire(Edgea5, Edgea6);
			//Wire1.Add(Wire2);
			//TopoDS_Shape Face = BRepBuilderAPI_MakeFace(Wire1);

			//GProp_GProps System;
			//BRepGProp::SurfaceProperties(Face, System);
			//auto max = System.MatrixOfInertia();
			//double Iyx = -max.Value(1, 2);
			//double Iyy = max.Value(2, 2);
			//double shang = (b-t1)*Iyx / Iyy;
			//尺寸标注
			//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();
			//OA Handle(AIS_LengthDimension) a18LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge8), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an18Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t1Value = a18LenDim->GetValue();
			//OA TCollection_AsciiString t1text = "DIM1 = ";
			//OA an18Aspect->SetValueStringFormat(t1text + t1Value);
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

			//OA Handle(AIS_LengthDimension) a12LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge1), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an12Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString aValue = a12LenDim->GetValue();
			//OA TCollection_AsciiString atext = "DIM3 = ";
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

			//OA Handle(AIS_LengthDimension) a23LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge2), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an23Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValue = a23LenDim->GetValue();
			//OA TCollection_AsciiString btext = "DIM4 = ";
			//OA an23Aspect->SetValueStringFormat(btext + bValue);
			//OA an23Aspect->ArrowAspect()->SetLength(0.2);
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

			//OA Handle(AIS_LengthDimension) aE26LenDim = new AIS_LengthDimension(Edge2, Edge6, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) anE26Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t2Value = aE26LenDim->GetValue();
			//OA TCollection_AsciiString t2text = "DIM2 = ";
			//OA anE26Aspect->SetValueStringFormat(t2text + t2Value);
			//OA anE26Aspect->ArrowAspect()->SetLength(0.2);
			//OA anE26Aspect->MakeArrows3d(Standard_False);
			//OA anE26Aspect->MakeText3d(Standard_False);
			//OA anE26Aspect->TextAspect()->SetHeight(0.3);
			//OA anE26Aspect->MakeTextShaded(Standard_True);
			//OA anE26Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA anE26Aspect->MakeUnitsDisplayed(Standard_False);
			//OA anE26Aspect->SetArrowTailSize(0.3);
			//OA anE26Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA aE26LenDim->SetTextPosition(PointE26);
			//OA aE26LenDim->SetDimensionAspect(anE26Aspect);
			//_geoWindow->getAisContext()->Display(aE26LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a23LenDim);
			//OA _aE26LenDim = aE26LenDim;

			//扭转常数
			double ti1, bi1, Ci1 = 1.0 / 3.0;
			bi1 = a;
			ti1 = t1;
			double It1 = Ci1 * bi1*pow(ti1, 3);
			double ti2, bi2, Ci2 = 1.0 / 3.0;
			bi2 = b - 2 * t1;
			ti2 = t2;
			double It2 = Ci2 * bi2*pow(ti2, 3);
			double Ita = 2 * It1 + It2;
			double ti3, bi3, Ci3 = 1.0 / 3.0;
			bi3 = b;
			ti3 = t2;
			double It3 = Ci3 * bi3*pow(ti3, 3);
			double ti4, bi4, Ci4 = 1.0 / 3.0;
			bi4 = a - t2;
			ti4 = t1;
			double It4 = Ci4 * bi4*pow(ti4, 3);
			double Itb = It3 + 2 * It4;
			double It = (Ita + Itb) / 2.0;

			double ShearCenterY = (a - t2 / 2.0) / 2 / (1 + (b - t1) / 6 / (a - t2 / 2.0)*t2 / t1);
			double ShearCenter = -(a / 2.0 - t2 / 2.0) - ShearCenterY;

			_data.push_back(It);
			_data.push_back(ShearCenter);
			_data.push_back(0);

			double SFY = (a - t2)*t1 * 2 / _data.at(0);
			double SFZ = (_data.at(0) - (a - t2)*t1 * 2 - t1 * t2*2.0) / _data.at(0);
			_data.push_back(SFY);
			_data.push_back(SFZ);
			/*emit showDataOnPanel(_data);*/
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
			QList<QVector2D> PointCoor;
			QVector2D C, D, E, F;
			C.setX(b / 2.0); C.setY(a / 2.0 - _data.at(1));
			D.setX(b / 2.0); D.setY(-a / 2.0 - _data.at(1));
			E.setX(-b / 2.0); E.setY(-a / 2.0 - _data.at(1));
			F.setX(-b / 2.0); F.setY(a / 2.0 - _data.at(1));
			PointCoor.push_back(C);
			PointCoor.push_back(D);
			PointCoor.push_back(E);
			PointCoor.push_back(F);
			mPBeam::getInstance()->_namePointListPair.insert(name, PointCoor);

			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(0, ShearCenter, Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//_geoWindow->getAisContext()->Display(shearMarker, true);
			//OA beamView::_nameAisPair.insert(name, shearMarker);
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
		}
		
	}

	void Channel2_SectionVisual::ReBuildChan2Sec(QList<double> para)
	{
		try
		{
			a = para.at(3);
			b = para.at(4);
			t1 = para.at(1);
			t2 = para.at(2);
			//_geoWindow->getAisContext()->EraseAll(true);
			para.removeFirst();
			_para = para;
	
			_X = -b / 2.0, _Y = a / 2.0, _Z = 0;
			//OA gp_Pnt Point1(_X, _Y, _Z); gp_Pnt Point2(_X, -_Y, _Z); gp_Pnt Point3(-_X, -_Y, _Z); gp_Pnt Point4(-_X, _Y, _Z);
			//OA gp_Pnt Point5(-_X - t1, _Y, _Z);	gp_Pnt Point8(_X + t1, _Y, _Z);
			//OA gp_Pnt Point6(-_X - t1, -_Y + t2, _Z); gp_Pnt Point7(_X + t1, -_Y + t2, _Z);
			//OA gp_Pnt Point18((2 * _X + t1) / 2, _Y + 0.5, _Z); gp_Pnt Point12(_X - 0.5, 0, 0); gp_Pnt Point23(0, -_Y - 0.5, 0);
			//OA gp_Pnt PointE26(-_X - t1, (-2 * _Y + t2) / 2, 0);
			//OA TopoDS_Edge Edge1 = BRepBuilderAPI_MakeEdge(Point1, Point2);
			//OA TopoDS_Edge Edge2 = BRepBuilderAPI_MakeEdge(Point2, Point3);
			//OA TopoDS_Edge Edge3 = BRepBuilderAPI_MakeEdge(Point3, Point4);
			//OA TopoDS_Edge Edge4 = BRepBuilderAPI_MakeEdge(Point4, Point5);
			//OA TopoDS_Edge Edge5 = BRepBuilderAPI_MakeEdge(Point5, Point6);
			//OA TopoDS_Edge Edge6 = BRepBuilderAPI_MakeEdge(Point6, Point7);
			//OA TopoDS_Edge Edge7 = BRepBuilderAPI_MakeEdge(Point7, Point8);
			//OA TopoDS_Edge Edge8 = BRepBuilderAPI_MakeEdge(Point8, Point1);
			//OA auto ChannelSecWire1 = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//OA TopoDS_Wire ChannelSecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6, Edge7, Edge8);
			//OA ChannelSecWire1.Add(ChannelSecWire2);
			//OA TopoDS_Shape ChannelSecFace = BRepBuilderAPI_MakeFace(ChannelSecWire1);
			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(ChannelSecFace);
			//OA _aisshape = aisshape;
			//OA BaseshowBeamSec(aisshape);

			QString name = mPBeam::getInstance()->_nameParaListPair.key(getPara());
			QList<QString> NAMElist = beamView::_nameAisPair.keys();
			if (!NAMElist.contains(name))
			{
				//emit AddSecSignal(name);
				//emit showParaOnPanel(_para);
			}
			else
			{
				beamView::_nameAisPair.remove(name);
				mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());
			}

			//OA beamView::_nameAisPair.insert(name, aisshape);
			//OA beamView::_secNameAisPair.insert(name, aisshape);
			//求截面数据
			//OA getDataFromBeam(name, ChannelSecFace);
			//剪切中心
			//尺寸标注
			//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();
			//OA Handle(AIS_LengthDimension) a18LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge8), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an18Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t1Value = a18LenDim->GetValue();
			//OA TCollection_AsciiString t1text = "DIM1 = ";
			//OA an18Aspect->SetValueStringFormat(t1text + t1Value);
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
			//OA TCollection_AsciiString aValue = a12LenDim->GetValue();
			//OA TCollection_AsciiString atext = "DIM3 = ";
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

			//OA Handle(AIS_LengthDimension) a23LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge2), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an23Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValue = a23LenDim->GetValue();
			//OA TCollection_AsciiString btext = "DIM4 = ";
			//OA an23Aspect->SetValueStringFormat(btext + bValue);
			//OA an23Aspect->ArrowAspect()->SetLength(0.2);
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

			//OA Handle(AIS_LengthDimension) aE26LenDim = new AIS_LengthDimension(Edge2, Edge6, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) anE26Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t2Value = aE26LenDim->GetValue();
			//OA TCollection_AsciiString t2text = "DIM2 = ";
			//OA anE26Aspect->SetValueStringFormat(t2text + t2Value);
			//OA anE26Aspect->ArrowAspect()->SetLength(0.2);
			//OA anE26Aspect->MakeArrows3d(Standard_False);
			//OA anE26Aspect->MakeText3d(Standard_False);
			//OA anE26Aspect->TextAspect()->SetHeight(0.3);
			//OA anE26Aspect->MakeTextShaded(Standard_True);
			//OA anE26Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA anE26Aspect->MakeUnitsDisplayed(Standard_False);
			//OA anE26Aspect->SetArrowTailSize(0.3);
			//OA anE26Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA aE26LenDim->SetTextPosition(PointE26);
			//OA aE26LenDim->SetDimensionAspect(anE26Aspect);
			//_geoWindow->getAisContext()->Display(aE26LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a23LenDim);
			//OA _a23LenDim = a23LenDim;

			//扭转常数
			double ti1, bi1, Ci1 = 1.0 / 3.0;

			bi1 = a;
			ti1 = t1;
			double It1 = Ci1 * bi1*pow(ti1, 3);
			double ti2, bi2, Ci2 = 1.0 / 3.0;
			bi2 = b - 2 * t1;
			ti2 = t2;
			double It2 = Ci2 * bi2*pow(ti2, 3);
			double Ita = 2 * It1 + It2;
			double ti3, bi3, Ci3 = 1.0 / 3.0;
			bi3 = b;
			ti3 = t2;
			double It3 = Ci3 * bi3*pow(ti3, 3);
			double ti4, bi4, Ci4 = 1.0 / 3.0;
			bi4 = a - t2;
			ti4 = t1;
			double It4 = Ci4 * bi4*pow(ti4, 3);
			double Itb = It3 + 2 * It4;
			double It = (Ita + Itb) / 2.0;
			/*	double ShearCenterY = pow(a - t2 / 2.0, 2)*pow(b - t1, 2)*t1 / 4.0 / _data.at(3);
				double ShearCenter = -(a / 2.0 - t2 / 2.0) - ShearCenterY;*/
				// double ShearCenterY = 2.0 / _data.at(3)*(b / 2.0 - t1 / 2.0)*(a - t2 / 2.0)*(a - t2 / 2.0)*t2 / 6.0*(2 * (b / 2.0 - t1 / 2) + (b / 2.0 - t1 / 2));
			double ShearCenterY = (a - t2 / 2.0) / 2 / (1 + (b - t1) / 6 / (a - t2 / 2.0)*t2 / t1);
			double ShearCenter = -(a / 2.0 - t2 / 2.0) - ShearCenterY;
			_data.push_back(It);
			_data.push_back(ShearCenter);
			_data.push_back(0);

			double SFY = (a - t2)*t1 * 2 / _data.at(0);
			double SFZ = (_data.at(0) - (a - t2)*t1 * 2 - t1 * t2*2.0) / _data.at(0);
			_data.push_back(SFY);
			_data.push_back(SFZ);
			/*emit showDataOnPanel(_data);*/
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(0, ShearCenter, Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//_geoWindow->getAisContext()->Display(shearMarker, true);
			//OA beamView::_nameAisPair.insert(name, shearMarker);
			//OA _shearMarker = shearMarker;
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
		}
		

	}
}
