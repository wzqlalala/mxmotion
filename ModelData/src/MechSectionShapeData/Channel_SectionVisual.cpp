#include "Channel_SectionVisual.h"
#include "mPBeam.h"
//#include <BRepBuilderAPI_MakeEdge.hxx>
//#include <TopoDS_Edge.hxx>
//#include <BRepBuilderAPI_MakeWire.hxx>
//#include <BRepBuilderAPI_MakeFace.hxx>
//#include <TopoDS_Wire.hxx>
//#include <Prs3d_DimensionTextHorizontalPosition.hxx>
//#include <AIS_LengthDimension.hxx>
//#include <GC_MakePlane.hxx>
#include <mGlobalSignals.h>

//#include "GeoWidget.h"
#include "beamView.h"
#include "Sample2D_Markers.h"

namespace MechSectionShapeData
{
	Channel_SectionVisual::Channel_SectionVisual(QString name, int id)
		: MechSectionShapeData::mSectionShapeBase(Sec_Channel,name,id)
	{
		_para.push_back(DIM1);
		_para.push_back(DIM2);
		_para.push_back(DIM3);
		_para.push_back(DIM4);
	}

	Channel_SectionVisual::~Channel_SectionVisual()
	{
	}
	Channel_SectionVisual* Channel_SectionVisual::clone() const
	{

		Channel_SectionVisual* newSec = new Channel_SectionVisual(_sectionShapeName, _id);

		newSec->DIM3 = DIM3;
		newSec->DIM4 = DIM4;
		newSec->DIM1 = DIM1;
		newSec->DIM2 = DIM2;
		newSec->_X = _X;
		newSec->_Y = _Y;
		newSec->_Z = _Z;


		newSec->_shearMarker = _shearMarker;
		newSec->_a14LenDim = _a14LenDim;
		newSec->_a12LenDim = _a12LenDim;
		newSec->_a84LenDim = _a84LenDim;
		newSec->_aE26LenDim = _aE26LenDim;
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
	void Channel_SectionVisual::CreateChanSec()
	{
		try
		{
			//OA gp_Pnt Point1(_X, _Y, _Z); gp_Pnt Point2(_X, -_Y, _Z); gp_Pnt Point3(-_X, -_Y, _Z); gp_Pnt Point4(-_X, _Y, _Z);
			//OA gp_Pnt Point5(_X + DIM3, _Y - DIM4, _Z);	gp_Pnt Point6(_X + DIM3, -(_Y - DIM4), _Z);
			//OA gp_Pnt Point7(-_X, -_Y + DIM4, _Z); gp_Pnt Point8(-_X, -(-_Y + DIM4), _Z);
			//OA gp_Pnt Point14(0, _Y + 0.5, _Z); gp_Pnt Point12(_X - 0.5, 0, _Z); gp_Pnt PointE26(_X + DIM3 / 2.0, -(_Y - DIM4) + 0.5, _Z); gp_Pnt Point84(-_X + 0.5, _Y - DIM4 / 2.0, _Z);
			//OA TopoDS_Edge Edge1 = BRepBuilderAPI_MakeEdge(Point4, Point1);
			//OA TopoDS_Edge Edge2 = BRepBuilderAPI_MakeEdge(Point1, Point2);
			//OA TopoDS_Edge Edge3 = BRepBuilderAPI_MakeEdge(Point2, Point3);
			//OA TopoDS_Edge Edge4 = BRepBuilderAPI_MakeEdge(Point3, Point7);
			//OA TopoDS_Edge Edge5 = BRepBuilderAPI_MakeEdge(Point7, Point6);
			//OA TopoDS_Edge Edge6 = BRepBuilderAPI_MakeEdge(Point6, Point5);
			//OA TopoDS_Edge Edge7 = BRepBuilderAPI_MakeEdge(Point5, Point8);
			//OA TopoDS_Edge Edge8 = BRepBuilderAPI_MakeEdge(Point8, Point4);
			//OA auto ChannelSecWire1 = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//OA TopoDS_Wire ChannelSecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6, Edge7, Edge8);
			//OA ChannelSecWire1.Add(ChannelSecWire2);
			//OA TopoDS_Shape ChannelSecFace = BRepBuilderAPI_MakeFace(ChannelSecWire1);
			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(ChannelSecFace);
			//OA _aisshape = aisshape;
			//OA BaseshowBeamSec(aisshape);

			QString name = mPBeam::getInstance()->setDefaultNameForBeam(Sec_Channel);
			/*emit AddSecSignal(name);
			emit showParaOnPanel(_para, name);*/
			//OA beamView::_nameAisPair.insert(name, aisshape);
			//OA beamView::_secNameAisPair.insert(name, aisshape);
			mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());
			//OA getDataFromBeam(name, ChannelSecFace);


			//尺寸标注
			//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();
			//OA Handle(AIS_LengthDimension) a14LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge1), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an14Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t1Value = a14LenDim->GetValue();
			//OA TCollection_AsciiString t1text = "DIM1 = ";
			//OA an14Aspect->SetValueStringFormat(t1text + t1Value);
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

			//OA Handle(AIS_LengthDimension) a12LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge2), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an12Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString aValue = a12LenDim->GetValue();
			//OA TCollection_AsciiString atext = "DIM2 = ";
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

			//OA Handle(AIS_LengthDimension) a84LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge8), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an84Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValue = a84LenDim->GetValue();
			//OA TCollection_AsciiString btext = "DIM4 = ";
			//OA an84Aspect->SetValueStringFormat(btext + bValue);
			//OA an84Aspect->ArrowAspect()->SetLength(0.2);
			//OA an84Aspect->MakeArrows3d(Standard_False);
			//OA an84Aspect->MakeText3d(Standard_False);
			//OA an84Aspect->TextAspect()->SetHeight(0.3);
			//OA an84Aspect->MakeTextShaded(Standard_True);
			//OA an84Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an84Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an84Aspect->SetArrowTailSize(0.3);
			//OA an84Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a84LenDim->SetTextPosition(Point84);
			//OA a84LenDim->SetDimensionAspect(an84Aspect);
			//_geoWindow->getAisContext()->Display(a84LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a84LenDim);
			//OA _a84LenDim = a84LenDim;

			//OA Handle(AIS_LengthDimension) aE26LenDim = new AIS_LengthDimension(Edge2, Edge6, aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) anE26Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t2Value = aE26LenDim->GetValue();
			//OA TCollection_AsciiString t2text = "DIM3 = ";
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
			//OA beamView::_nameAisPair.insert(name, aE26LenDim);
			//OA _aE26LenDim = aE26LenDim;

			//扭转常数
			double ti1, bi1, Ci1 = 1.0 / 3.0;
			bi1 = DIM1;
			ti1 = DIM4;
			double It1 = Ci1 * bi1*pow(ti1, 3);
			double ti2, bi2, Ci2 = 1.0 / 3.0;
			bi2 = DIM2 - 2 * DIM4;
			ti2 = DIM3;
			double It2 = Ci2 * bi2*pow(ti2, 3);
			double Ita = 2 * It1 + It2;
			double ti3, bi3, Ci3 = 1.0 / 3.0;
			bi3 = DIM2;
			ti3 = DIM3;
			double It3 = Ci3 * bi3*pow(ti3, 3);
			double ti4, bi4, Ci4 = 1.0 / 3.0;
			bi4 = DIM1 - DIM3;
			ti4 = DIM4;
			double It4 = Ci4 * bi4*pow(ti4, 3);
			double Itb = It3 + 2 * It4;
			double It = (Ita + Itb) / 2.0;

			double ShearCenterZ = (DIM1 - DIM3 / 2.0) / 2 / (1 + (DIM2 - DIM4) / 6 / (DIM1 - DIM3 / 2.0)*DIM3 / DIM4);
			double ShearCenter = -(DIM1 / 2.0 - DIM3 / 2.0) - ShearCenterZ;

			_data.push_back(It);
			_data.push_back(0);
			_data.push_back(ShearCenter);


			double SFZ = (DIM1 - DIM3)*DIM4 * 2 / _data.at(0);
			double SFY = (_data.at(0) - (DIM1 - DIM3)*DIM4 * 2 - DIM4 * DIM3*2.0) / _data.at(0);
			_data.push_back(SFY);
			_data.push_back(SFZ);
			//emit showDataOnPanel(_data);
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
			QList<QVector2D> PointCoor;
			QVector2D C, D, E, F;
			C.setY(DIM2 / 2.0); C.setX(DIM1 / 2.0 - _data.at(1));
			D.setY(DIM2 / 2.0); D.setX(-DIM1 / 2.0 - _data.at(1));
			E.setY(-DIM2 / 2.0); E.setX(-DIM1 / 2.0 - _data.at(1));
			F.setY(-DIM2 / 2.0); F.setX(DIM1 / 2.0 - _data.at(1));
			PointCoor.push_back(C);
			PointCoor.push_back(D);
			PointCoor.push_back(E);
			PointCoor.push_back(F);
			mPBeam::getInstance()->_namePointListPair.insert(name, PointCoor);
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

	void Channel_SectionVisual::ReBuildChanSec(QList<double> para)
	{
		DIM1 = para.at(1);
		DIM2 = para.at(2);
		DIM3 = para.at(3);
		DIM4 = para.at(4);
		//_geoWindow->getAisContext()->EraseAll(true);
		para.removeFirst();
		_para = para;

		_X = -DIM1 / 2.0, _Y = DIM2 / 2.0, _Z = 0;
		//OA gp_Pnt Point1(_X, _Y, _Z); gp_Pnt Point2(_X, -_Y, _Z); gp_Pnt Point3(-_X, -_Y, _Z); gp_Pnt Point4(-_X, _Y, _Z);
		//OA gp_Pnt Point5(_X + DIM3, _Y - DIM4, _Z);	gp_Pnt Point6(_X + DIM3, -(_Y - DIM4), _Z);
		//OA gp_Pnt Point7(-_X, -_Y + DIM4, _Z); gp_Pnt Point8(-_X, -(-_Y + DIM4), _Z);
		//OA gp_Pnt Point14(0, _Y + 0.5, _Z); gp_Pnt Point12(_X - 0.5, 0, _Z); gp_Pnt PointE26(_X + DIM3 / 2.0, -(_Y - DIM4) + 0.5, _Z); gp_Pnt Point84(-_X + 0.5, _Y - DIM4 / 2.0, _Z);
		//OA TopoDS_Edge Edge1 = BRepBuilderAPI_MakeEdge(Point4, Point1);
		//OA TopoDS_Edge Edge2 = BRepBuilderAPI_MakeEdge(Point1, Point2);
		//OA TopoDS_Edge Edge3 = BRepBuilderAPI_MakeEdge(Point2, Point3);
		//OA TopoDS_Edge Edge4 = BRepBuilderAPI_MakeEdge(Point3, Point7);
		//OA TopoDS_Edge Edge5 = BRepBuilderAPI_MakeEdge(Point7, Point6);
		//OA TopoDS_Edge Edge6 = BRepBuilderAPI_MakeEdge(Point6, Point5);
		//OA TopoDS_Edge Edge7 = BRepBuilderAPI_MakeEdge(Point5, Point8);
		//OA TopoDS_Edge Edge8 = BRepBuilderAPI_MakeEdge(Point8, Point4);
		//OA auto ChannelSecWire1 = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
		//OA TopoDS_Wire ChannelSecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6, Edge7, Edge8);
		//OA ChannelSecWire1.Add(ChannelSecWire2);
		//OA TopoDS_Shape ChannelSecFace = BRepBuilderAPI_MakeFace(ChannelSecWire1);
		//OA Handle(AIS_Shape) aisshape = new AIS_Shape(ChannelSecFace);
		//OA _aisshape = aisshape;
		//OA BaseshowBeamSec(aisshape);

		QString name =  mPBeam::getInstance()->_nameParaListPair.key(getPara());;
		QList<QString> NAMElist = beamView::_nameAisPair.keys();
		if (!NAMElist.contains(name))
		{
			/*emit AddSecSignal(name);
			emit showParaOnPanel(_para);*/
		}
		else
		{
			//OA beamView::_nameAisPair.remove(name);
		
			 mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());
		}
		//OA beamView::_nameAisPair.insert(name, aisshape);
		//OA beamView::_secNameAisPair.insert(name, aisshape);
		//OA getDataFromBeam(name, ChannelSecFace);

		//尺寸标注
		//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
		//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();
		//OA Handle(AIS_LengthDimension) a14LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge1), aPlane->Pln());
		//OA Handle(Prs3d_DimensionAspect) an14Aspect = new Prs3d_DimensionAspect();
		//OA TCollection_AsciiString t1Value = a14LenDim->GetValue();
		//OA TCollection_AsciiString t1text = "DIM1 = ";
		//OA an14Aspect->SetValueStringFormat(t1text + t1Value);
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

		//OA Handle(AIS_LengthDimension) a12LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge2), aPlane->Pln());
		//OA Handle(Prs3d_DimensionAspect) an12Aspect = new Prs3d_DimensionAspect();
		//OA TCollection_AsciiString aValue = a12LenDim->GetValue();
		//OA TCollection_AsciiString atext = "DIM2 = ";
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

		//OA Handle(AIS_LengthDimension) a84LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge8), aPlane->Pln());
		//OA Handle(Prs3d_DimensionAspect) an84Aspect = new Prs3d_DimensionAspect();
		//OA TCollection_AsciiString bValue = a84LenDim->GetValue();
		//OA TCollection_AsciiString btext = "DIM4 = ";
		//OA an84Aspect->SetValueStringFormat(btext + bValue);
		//OA an84Aspect->ArrowAspect()->SetLength(0.2);
		//OA an84Aspect->MakeArrows3d(Standard_False);
		//OA an84Aspect->MakeText3d(Standard_False);
		//OA an84Aspect->TextAspect()->SetHeight(0.3);
		//OA an84Aspect->MakeTextShaded(Standard_True);
		//OA an84Aspect->SetCommonColor(Quantity_NOC_BLACK);
		//OA an84Aspect->MakeUnitsDisplayed(Standard_False);
		//OA an84Aspect->SetArrowTailSize(0.3);
		//OA an84Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
		//OA a84LenDim->SetTextPosition(Point84);
		//OA a84LenDim->SetDimensionAspect(an84Aspect);
		//_geoWindow->getAisContext()->Display(a84LenDim, true);
		//OA beamView::_nameAisPair.insert(name, a84LenDim);
		//OA _a84LenDim = a84LenDim;

		//OA Handle(AIS_LengthDimension) aE26LenDim = new AIS_LengthDimension(Edge2, Edge6, aPlane->Pln());
		//OA Handle(Prs3d_DimensionAspect) anE26Aspect = new Prs3d_DimensionAspect();
		//OA TCollection_AsciiString t2Value = aE26LenDim->GetValue();
		//OA TCollection_AsciiString t2text = "DIM3 = ";
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
		//OA beamView::_nameAisPair.insert(name, aE26LenDim);
		//OA _aE26LenDim = aE26LenDim;

		//扭转常数
		double ti1, bi1, Ci1 = 1.0 / 3.0;
		bi1 = DIM1;
		ti1 = DIM4;
		double It1 = Ci1 * bi1*pow(ti1, 3);
		double ti2, bi2, Ci2 = 1.0 / 3.0;
		bi2 = DIM2 - 2 * DIM4;
		ti2 = DIM3;
		double It2 = Ci2 * bi2*pow(ti2, 3);
		double Ita = 2 * It1 + It2;
		double ti3, bi3, Ci3 = 1.0 / 3.0;
		bi3 = DIM2;
		ti3 = DIM3;
		double It3 = Ci3 * bi3*pow(ti3, 3);
		double ti4, bi4, Ci4 = 1.0 / 3.0;
		bi4 = DIM1 - DIM3;
		ti4 = DIM4;
		double It4 = Ci4 * bi4*pow(ti4, 3);
		double Itb = It3 + 2 * It4;
		double It = (Ita + Itb) / 2.0;

		double ShearCenterZ = (DIM1 - DIM3 / 2.0) / 2 / (1 + (DIM2 - DIM4) / 6 / (DIM1 - DIM3 / 2.0)*DIM3 / DIM4);
		double ShearCenter = -(DIM1 / 2.0 - DIM3 / 2.0) - ShearCenterZ;

		_data.push_back(It);
		_data.push_back(0);
		_data.push_back(ShearCenter);

		double SFZ = (DIM1 - DIM3)*DIM4 * 2 / _data.at(0);
		double SFY = (_data.at(0) - (DIM1 - DIM3)*DIM4 * 2 - DIM4 * DIM3*2.0) / _data.at(0);
		_data.push_back(SFY);
		_data.push_back(SFZ);
		//emit showDataOnPanel(_data);
		 mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
		QList<QVector2D> PointCoor;
		QVector2D C, D, E, F;
		C.setY(DIM2 / 2.0); C.setX(DIM1 / 2.0 - _data.at(1));
		D.setY(DIM2 / 2.0); D.setX(-DIM1 / 2.0 - _data.at(1));
		E.setY(-DIM2 / 2.0); E.setX(-DIM1 / 2.0 - _data.at(1));
		F.setY(-DIM2 / 2.0); F.setX(DIM1 / 2.0 - _data.at(1));
		PointCoor.push_back(C);
		PointCoor.push_back(D);
		PointCoor.push_back(E);
		PointCoor.push_back(F);
		 mPBeam::getInstance()->_namePointListPair.insert(name, PointCoor);
		//OA Handle(Sample2D_Markers) shearMarker =
			//OA new Sample2D_Markers(ShearCenter, 0, Aspect_TOM_O_X, Quantity_NOC_YELLOW);
		//_geoWindow->getAisContext()->Display(shearMarker, true);
		//OA beamView::_nameAisPair.insert(name, shearMarker);
		//OA _shearMarker = shearMarker;
	}
}
