#include "Channel1_SectionVisual.h"
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
#include <mGlobalSignals.h>

//#include "GeoWidget.h"
#include "beamView.h"

namespace MechSectionShapeData
{
	Channel1_SectionVisual::Channel1_SectionVisual(QString name, int id)
		: MechSectionShapeData::mSectionShapeBase(Sec_Channel1,name,id)
	{

		_para.push_back(t1);
		_para.push_back(t2);
		_para.push_back(b);
		_para.push_back(a);
	
	}

	Channel1_SectionVisual::~Channel1_SectionVisual()
	{
	}
	Channel1_SectionVisual* Channel1_SectionVisual::clone() const
	{
		Channel1_SectionVisual* newSec = new Channel1_SectionVisual(_sectionShapeName, _id);

		newSec->a = a;
		newSec->b = b;
		newSec->t2 = t2;
		newSec->t1 = t1;
		newSec->_X = _X;
		newSec->_Y = _Y;
		newSec->_Z = _Z;


		//OA newSec->_shearMarker = _shearMarker;
		//OA newSec->_aR18LenDim = _aR18LenDim;
		//OA newSec->_aL18LenDim = _aL18LenDim;
		//OA newSec->_a16LenDim = _a16LenDim;
		//OA newSec->_a25LenDim = _a25LenDim;
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
	void Channel1_SectionVisual::CreateChan1Sec()
	{
		try
		{	
			//OA gp_Pnt Point8(-t2, _Y, _Z); /*----------------------*/ gp_Pnt Point1(_X, _Y, _Z);
			/*|						    						  |							*/
			//OA /*|*/					   gp_Pnt Point3(0, b / 2.0, _Z); gp_Pnt Point2(_X, b/2.0, _Z);
			/*|						   |													*/	
			//OA /*|*/					   gp_Pnt Point4(0, -(b / 2.0), _Z); gp_Pnt Point5(_X, -(b / 2.0), _Z);
			/*|													  |							*/
			//OA gp_Pnt Point7(-t2, -_Y, _Z); /*----------------------*/ gp_Pnt Point6(_X, -_Y, _Z);

			//DIM1、DIM2共用一条边，额外设置点进行拆分
			//OA gp_Pnt PointHalf(0, _Y, _Z);
			
			//OA gp_Pnt PointH1(_X * 0.5, _Y + 0.5, _Z);
			//OA gp_Pnt Point8H(-t2 * 0.5, _Y + 0.5, _Z);
			//OA gp_Pnt Point25(_X + 0.5, 0.5, _Z);
			//OA gp_Pnt Point16(_X + 1.0, -0.5, _Z);


			//OA TopoDS_Edge EdgeLeft = BRepBuilderAPI_MakeEdge(Point8, PointHalf);
			//OA TopoDS_Edge EdgeRight = BRepBuilderAPI_MakeEdge(PointHalf, Point1);

			
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
			QString name = mPBeam::getInstance()->setDefaultNameForBeam(Sec_Channel1);
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
			
			//OA Handle(AIS_LengthDimension) aR18LenDim = new AIS_LengthDimension(TopoDS::Edge(EdgeRight), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) anR18Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t1Value = aR18LenDim->GetValue();
			//OA TCollection_AsciiString t1text = "DIM1 = ";
			//OA anR18Aspect->SetValueStringFormat(t1text + t1Value);
			//OA anR18Aspect->MakeArrows3d(Standard_False);
			//OA anR18Aspect->MakeText3d(Standard_False);
			//OA anR18Aspect->TextAspect()->SetHeight(0.3);
			//OA anR18Aspect->ArrowAspect()->SetLength(0.2);
			//OA anR18Aspect->MakeTextShaded(Standard_True);
			//OA anR18Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA anR18Aspect->MakeUnitsDisplayed(Standard_False);
			//OA anR18Aspect->SetArrowTailSize(0.3);
			//OA anR18Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA aR18LenDim->SetTextPosition(PointH1);
			//OA aR18LenDim->SetDimensionAspect(anR18Aspect);
			//_geoWindow->getAisContext()->Display(a18LenDim, true);
			//OA beamView::_nameAisPair.insert(name, aR18LenDim);
			//OA _aR18LenDim = aR18LenDim;

			//OA Handle(AIS_LengthDimension) aL18LenDim = new AIS_LengthDimension(TopoDS::Edge(EdgeLeft), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) aL18Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString aValue = aL18LenDim->GetValue();
			//OA TCollection_AsciiString atext = "DIM2 = ";
			//OA aL18Aspect->SetValueStringFormat(atext + aValue);
			//OA aL18Aspect->MakeArrows3d(Standard_False);
			//OA aL18Aspect->MakeText3d(Standard_False);
			//OA aL18Aspect->TextAspect()->SetHeight(0.3);
			//OA aL18Aspect->ArrowAspect()->SetLength(0.2);
			//OA aL18Aspect->MakeTextShaded(Standard_True);
			//OA aL18Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA aL18Aspect->MakeUnitsDisplayed(Standard_False);
			//OA aL18Aspect->SetArrowTailSize(0.3);
			//OA aL18Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA aL18LenDim->SetTextPosition(Point8H);
			//OA aL18LenDim->SetDimensionAspect(aL18Aspect);
			//_geoWindow->getAisContext()->Display(a12LenDim, true);
			//OA beamView::_nameAisPair.insert(name, aL18LenDim);
			//OA _aL18LenDim = aL18LenDim;

			//OA Handle(AIS_LengthDimension) a25LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge3), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an25Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValue = a25LenDim->GetValue();
			//OA TCollection_AsciiString btext = "DIM3 = ";
			//OA an25Aspect->SetValueStringFormat(btext + bValue);
			//OA an25Aspect->ArrowAspect()->SetLength(0.2);
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
			//_geoWindow->getAisContext()->Display(a23LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a25LenDim);
			//OA _a25LenDim = a25LenDim;

			//OA Handle(AIS_LengthDimension) aE16LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge7), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) anE16Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t2Value = aE16LenDim->GetValue();
			//OA TCollection_AsciiString t2text = "DIM4 = ";
			//OA anE16Aspect->SetValueStringFormat(t2text + t2Value);
			//OA anE16Aspect->ArrowAspect()->SetLength(0.2);
			//OA anE16Aspect->MakeArrows3d(Standard_False);
			//OA anE16Aspect->MakeText3d(Standard_False);
			//OA anE16Aspect->TextAspect()->SetHeight(0.3);
			//OA anE16Aspect->MakeTextShaded(Standard_True);
			//OA anE16Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA anE16Aspect->MakeUnitsDisplayed(Standard_False);
			//OA anE16Aspect->SetArrowTailSize(0.3);
			//OA anE16Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA aE16LenDim->SetTextPosition(Point16);
			//OA aE16LenDim->SetDimensionAspect(anE16Aspect);
			//_geoWindow->getAisContext()->Display(aE26LenDim, true);
			//OA beamView::_nameAisPair.insert(name, aE16LenDim);
			//OA _a16LenDim = aE16LenDim;

			//扭转常数
			double ti1, bi1, Ci1 = 1.0 / 3.0;
			bi1 = t1 + t2;
			ti1 = (a-b)/2.0;
			double It1 = Ci1 * bi1 * pow(ti1, 3);
			double ti2, bi2, Ci2 = 1.0 / 3.0;
			bi2 = b;
			ti2 = t2;
			double It2 = Ci2 * bi2 * pow(ti2, 3);
			double Ita = 2 * It1 + It2;
			double ti3, bi3, Ci3 = 1.0 / 3.0;
			bi3 = t1;
			ti3 = (a - b) / 2.0;
			double It3 = Ci3 * bi3 * pow(ti3, 3);
			double ti4, bi4, Ci4 = 1.0 / 3.0;
			bi4 = a;
			ti4 = t2;
			double It4 = Ci4 * bi4 * pow(ti4, 3);
			double Itb = It4 + 2 * It3;
			double It = (Ita + Itb) / 2.0;
			double ShearCenterZ = (t1 + t2 * 0.5) / 2 / (1 + (a - (a - b) / 2.0) / 6 / (t1 + t2 * 0.5) * t2 / ((a - b) / 2.0));
			double ShearCenter = -((t1 + t2) / 2.0 - t2 / 2.0) - ShearCenterZ;
			_data.push_back(It);
			_data.push_back(ShearCenter);
			_data.push_back(0);

			double SFZ = ((a - b) / 2.0) * t1 * 2 / _data.at(0);
			double SFY = (_data.at(0) - ((a - b) / 2.0) * t1 * 2 - ((a - b) / 2.0) * t2 * 2.0) / _data.at(0);
			_data.push_back(SFY);
			_data.push_back(SFZ);
			/*emit showDataOnPanel(_data);*/
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
			QList<QVector2D> PointCoor;
			QVector2D C, D, E, F;
			C.setX((t1 + t2) / 2.0 - _data.at(1)); C.setY(a / 2.0);
			D.setX(-(t1 + t2) / 2.0 - _data.at(1)); D.setY(a / 2.0);
			E.setX(-(t1 + t2) / 2.0 - _data.at(1)); E.setY(-a / 2.0);
			F.setX((t1 + t2) / 2.0 - _data.at(1)); F.setY(-a / 2.0);
			PointCoor.push_back(C);
			PointCoor.push_back(D);
			PointCoor.push_back(E);
			PointCoor.push_back(F);
			mPBeam::getInstance()->_namePointListPair.insert(name, PointCoor);

			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(ShearCenter, 0, Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//_geoWindow->getAisContext()->Display(shearMarker, true);
			//OA beamView::_nameAisPair.insert(name, shearMarker);
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
		}

	}

	void Channel1_SectionVisual::ReBuildChan1Sec(QList<double> para)
	{
		try
		{
			a = para.at(4);
			b = para.at(3);
			t1 = para.at(1);
			t2 = para.at(2);
			//_geoWindow->getAisContext()->EraseAll(true);
			para.removeFirst();
			_para = para;
	
			_X = t1, _Y = a / 2.0, _Z = 0;

			//OA gp_Pnt Point8(-t2, _Y, _Z); /*----------------------*/ gp_Pnt Point1(_X, _Y, _Z);
			/*|						    						  |							*/
			//OA /*|*/					   gp_Pnt Point3(0, b / 2.0, _Z); gp_Pnt Point2(_X, b / 2.0, _Z);
			/*|						   |													*/
			//OA /*|*/					   gp_Pnt Point4(0, -(b / 2.0), _Z); gp_Pnt Point5(_X, -(b / 2.0), _Z);
			/*|													  |							*/
			//OA gp_Pnt Point7(-t2, -_Y, _Z); /*----------------------*/ gp_Pnt Point6(_X, -_Y, _Z);

			//DIM1、DIM2共用一条边，额外设置点进行拆分
			//OA gp_Pnt PointHalf(0, _Y, _Z);

			//OA gp_Pnt PointH1(_X * 0.5, _Y + 0.5, _Z);
			//OA gp_Pnt Point8H(-t2 * 0.5, _Y + 0.5, _Z);
			//OA gp_Pnt Point25(_X + 0.5, 0.5, _Z);
			//OA gp_Pnt Point16(_X + 1.0, -0.5, _Z);


			//OA TopoDS_Edge EdgeLeft = BRepBuilderAPI_MakeEdge(Point8, PointHalf);
			//OA TopoDS_Edge EdgeRight = BRepBuilderAPI_MakeEdge(PointHalf, Point1);


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

			//OA Handle(AIS_LengthDimension) aR18LenDim = new AIS_LengthDimension(TopoDS::Edge(EdgeRight), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) anR18Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t1Value = aR18LenDim->GetValue();
			//OA TCollection_AsciiString t1text = "DIM1 = ";
			//OA anR18Aspect->SetValueStringFormat(t1text + t1Value);
			//OA anR18Aspect->MakeArrows3d(Standard_False);
			//OA anR18Aspect->MakeText3d(Standard_False);
			//OA anR18Aspect->TextAspect()->SetHeight(0.3);
			//OA anR18Aspect->ArrowAspect()->SetLength(0.2);
			//OA anR18Aspect->MakeTextShaded(Standard_True);
			//OA anR18Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA anR18Aspect->MakeUnitsDisplayed(Standard_False);
			//OA anR18Aspect->SetArrowTailSize(0.3);
			//OA anR18Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA aR18LenDim->SetTextPosition(PointH1);
			//OA aR18LenDim->SetDimensionAspect(anR18Aspect);
			//_geoWindow->getAisContext()->Display(a18LenDim, true);
			//OA beamView::_nameAisPair.insert(name, aR18LenDim);
			//OA _aR18LenDim = aR18LenDim;

			//OA Handle(AIS_LengthDimension) aL18LenDim = new AIS_LengthDimension(TopoDS::Edge(EdgeLeft), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) aL18Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString aValue = aL18LenDim->GetValue();
			//OA TCollection_AsciiString atext = "DIM2 = ";
			//OA aL18Aspect->SetValueStringFormat(atext + aValue);
			//OA aL18Aspect->MakeArrows3d(Standard_False);
			//OA aL18Aspect->MakeText3d(Standard_False);
			//OA aL18Aspect->TextAspect()->SetHeight(0.3);
			//OA aL18Aspect->ArrowAspect()->SetLength(0.2);
			//OA aL18Aspect->MakeTextShaded(Standard_True);
			//OA aL18Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA aL18Aspect->MakeUnitsDisplayed(Standard_False);
			//OA aL18Aspect->SetArrowTailSize(0.3);
			//OA aL18Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA aL18LenDim->SetTextPosition(Point8H);
			//OA aL18LenDim->SetDimensionAspect(aL18Aspect);
			//_geoWindow->getAisContext()->Display(a12LenDim, true);
			//OA beamView::_nameAisPair.insert(name, aL18LenDim);
			//OA _aL18LenDim = aL18LenDim;

			//OA Handle(AIS_LengthDimension) a25LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge3), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an25Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValue = a25LenDim->GetValue();
			//OA TCollection_AsciiString btext = "DIM3 = ";
			//OA an25Aspect->SetValueStringFormat(btext + bValue);
			//OA an25Aspect->ArrowAspect()->SetLength(0.2);
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
			//_geoWindow->getAisContext()->Display(a23LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a25LenDim);
			//OA _a25LenDim = a25LenDim;

			//OA Handle(AIS_LengthDimension) aE16LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge7), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) anE16Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t2Value = aE16LenDim->GetValue();
			//OA TCollection_AsciiString t2text = "DIM4 = ";
			//OA anE16Aspect->SetValueStringFormat(t2text + t2Value);
			//OA anE16Aspect->ArrowAspect()->SetLength(0.2);
			//OA anE16Aspect->MakeArrows3d(Standard_False);
			//OA anE16Aspect->MakeText3d(Standard_False);
			//OA anE16Aspect->TextAspect()->SetHeight(0.3);
			//OA anE16Aspect->MakeTextShaded(Standard_True);
			//OA anE16Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA anE16Aspect->MakeUnitsDisplayed(Standard_False);
			//OA anE16Aspect->SetArrowTailSize(0.3);
			//OA anE16Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA aE16LenDim->SetTextPosition(Point16);
			//OA aE16LenDim->SetDimensionAspect(anE16Aspect);
			//_geoWindow->getAisContext()->Display(aE26LenDim, true);
			//OA beamView::_nameAisPair.insert(name, aE16LenDim);
			//OA _a16LenDim = aE16LenDim;

			//扭转常数
			double ti1, bi1, Ci1 = 1.0 / 3.0;
			bi1 = t1 + t2;
			ti1 = (a - b) / 2.0;
			double It1 = Ci1 * bi1 * pow(ti1, 3);
			double ti2, bi2, Ci2 = 1.0 / 3.0;
			bi2 = b;
			ti2 = t2;
			double It2 = Ci2 * bi2 * pow(ti2, 3);
			double Ita = 2 * It1 + It2;
			double ti3, bi3, Ci3 = 1.0 / 3.0;
			bi3 = t1;
			ti3 = (a - b) / 2.0;
			double It3 = Ci3 * bi3 * pow(ti3, 3);
			double ti4, bi4, Ci4 = 1.0 / 3.0;
			bi4 = a;
			ti4 = t2;
			double It4 = Ci4 * bi4 * pow(ti4, 3);
			double Itb = It4 + 2 * It3;
			double It = (Ita + Itb) / 2.0;
			/*	double ShearCenterY = pow(a - t2 / 2.0, 2)*pow(b - t1, 2)*t1 / 4.0 / _data.at(3);
				double ShearCenter = -(a / 2.0 - t2 / 2.0) - ShearCenterY;*/
				// double ShearCenterY = 2.0 / _data.at(3)*(b / 2.0 - t1 / 2.0)*(a - t2 / 2.0)*(a - t2 / 2.0)*t2 / 6.0*(2 * (b / 2.0 - t1 / 2) + (b / 2.0 - t1 / 2));
			double ShearCenterZ = (t1 + t2 * 0.5) / 2 / (1 + (a - (a - b) / 2.0) / 6 / (t1 + t2 * 0.5) * t2 / ((a - b) / 2.0));
			double ShearCenter = -((t1 + t2) / 2.0 - t2 / 2.0) - ShearCenterZ;
			_data.push_back(It);
			_data.push_back(ShearCenter);
			_data.push_back(0);

			double SFZ = ((a - b) / 2.0)*t1 * 2 / _data.at(0);
			double SFY = (_data.at(0) - ((a - b) / 2.0) * t1 * 2 - ((a - b) / 2.0) * t2*2.0) / _data.at(0);
			_data.push_back(SFY);
			_data.push_back(SFZ);
			/*emit showDataOnPanel(_data);*/
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(ShearCenter, 0, Aspect_TOM_O_X, Quantity_NOC_YELLOW);
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

