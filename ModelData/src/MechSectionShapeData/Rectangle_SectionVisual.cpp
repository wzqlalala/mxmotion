#include "Rectangle_SectionVisual.h"
//#include <gp.hxx>
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
	Rectangle_SectionVisual::Rectangle_SectionVisual(QString name, int id)
		:mSectionShapeBase(Sec_Rec,name,id)
	{

		_para.push_back(a);
		_para.push_back(b);
	}

	Rectangle_SectionVisual::~Rectangle_SectionVisual()
	{
	}
	Rectangle_SectionVisual* Rectangle_SectionVisual::clone() const
	{
		//OA Handle(Sample2D_Markers) _shearMarker;
		//OA Handle(AIS_LengthDimension) _a14LenDim;
		//OA Handle(AIS_LengthDimension) _a12LenDim;


		//OA Handle(AIS_Shape)  _aisshape;



		Rectangle_SectionVisual* newSec = new Rectangle_SectionVisual(_sectionShapeName, _id);

		newSec->a = a;
		newSec->b = b;
		newSec->_X = _X;
		newSec->_Y = _Y;
		newSec->_Z = _Z;


		//OA newSec->_shearMarker = _shearMarker;
		//OA newSec->_a14LenDim = _a14LenDim;
		//OA newSec->_a12LenDim = _a12LenDim;
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
	void Rectangle_SectionVisual::CreatRecSec()
	{
		try
		{
			//OA gp_Pnt Point1(_X, _Y, _Z);
			//OA gp_Pnt Point2(_X, -_Y, _Z);
			//OA gp_Pnt Point3(-_X, -_Y, _Z);
			//OA gp_Pnt Point4(-_X, _Y, _Z);
			//OA gp_Pnt Point14(0, _Y + 0.5, _Z);
			//OA gp_Pnt Point12(_X - 0.5, 0, _Z);
			//OA auto Edge1 = BRepBuilderAPI_MakeEdge(Point1, Point2);
			//OA auto Edge2 = BRepBuilderAPI_MakeEdge(Point2, Point3);
			//OA auto Edge3 = BRepBuilderAPI_MakeEdge(Point3, Point4);
			//OA auto Edge4 = BRepBuilderAPI_MakeEdge(Point4, Point1);
			//OA TopoDS_Wire RecSecWire = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//OA TopoDS_Shape RecSecFace = BRepBuilderAPI_MakeFace(RecSecWire);

			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(RecSecFace);
			//OA _aisshape = aisshape;
			//OA BaseshowBeamSec(aisshape);
			QString name = mPBeam::getInstance()->setDefaultNameForBeam(Sec_Rec);
			/*emit AddSecSignal(name);
			emit showParaOnPanel(_para, name);*/
			//OA beamView::_nameAisPair.insert(name, aisshape);
			//OA beamView::_secNameAisPair.insert(name, aisshape);
			mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());
			//OA getDataFromBeam(name, RecSecFace);
			//OA GProp_GProps System;
			//OA BRepGProp::SurfaceProperties(RecSecFace, System);
			//OA auto CentreOfMass = System.CentreOfMass();
			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(CentreOfMass.X(), CentreOfMass.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//_geoWindow->getAisContext()->Display(shearMarker, Standard_False);
			//OA beamView::_nameAisPair.insert(name, shearMarker);
			//OA _shearMarker = shearMarker;

			//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();

			//OA Handle(AIS_LengthDimension) a14LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge4), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an14Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString aValue = a14LenDim->GetValue();
			//OA TCollection_AsciiString atext = "DIM1 = ";
			//OA an14Aspect->SetValueStringFormat(atext + aValue);
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

			//OA Handle(AIS_LengthDimension) a12LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge1), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an12Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValue = a12LenDim->GetValue();
			//OA TCollection_AsciiString btext = "DIM2 = ";
			//OA an12Aspect->SetValueStringFormat(btext + bValue);
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
			//求抗扭截面系数
			double t, bi, Ci;
			if (a < b)
			{
				t = a;
				bi = b;
			}
			else
			{
				bi = a;
				t = b;
			}
			if (t / bi < 0.1)
			{
				Ci = 1 / 3;
			}
			else
			{
				Ci = (1.0 / 3.0) * (1.0 - 0.630 *t / bi + 0.052*pow((t / bi), 5));

			}
			double It = Ci * bi*pow(t, 3);
			// double It = (bi*pow(t, 3) / 16.0)*(16.0 / 3.0 - (3.36*t / bi)*(1 - pow(t, 4) / 12.0 / pow(bi, 4)));
			_data.push_back(It);
			//OA _data.push_back(CentreOfMass.Y());
			//OA _data.push_back(CentreOfMass.X());
			_data.push_back(5.0 / 6.0);
			_data.push_back(5.0 / 6.0);
			//emit showDataOnPanel(_data);
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);

			QList<QVector2D> PointCoor;
			QVector2D C, D, E, F;
			C.setX(a / 2.0); C.setY(b / 2.0);
			D.setX(a / 2.0); D.setY(b / 2.0);
			E.setX(a / 2.0); E.setY(b / 2.0);
			F.setX(a / 2.0); F.setY(b / 2.0);
			PointCoor.push_back(C);
			PointCoor.push_back(D);
			PointCoor.push_back(E);
			PointCoor.push_back(F);
			mPBeam::getInstance()->_namePointListPair.insert(name, PointCoor);
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
		}
		
	}

	void Rectangle_SectionVisual::ReBuildRecSec(QList<double> para)
	{
		try
		{
			a = para.at(1);
			b = para.at(2);
			//_geoWindow->getAisContext()->EraseAll(true);
			para.removeFirst();
			_para = para;

			_Y = b / 2, _X = -a / 2, _Z = 0;
			//OA gp_Pnt Point1(_X, _Y, _Z);
			//OA gp_Pnt Point2(_X, -_Y, _Z);
			//OA gp_Pnt Point3(-_X, -_Y, _Z);
			//OA gp_Pnt Point4(-_X, _Y, _Z);
			//OA gp_Pnt Point14(0, _Y + 0.5, _Z);
			//OA gp_Pnt Point12(_X - 0.5, 0, _Z);
			//OA auto Edge1 = BRepBuilderAPI_MakeEdge(Point1, Point2);
			//OA auto Edge2 = BRepBuilderAPI_MakeEdge(Point2, Point3);
			//OA auto Edge3 = BRepBuilderAPI_MakeEdge(Point3, Point4);
			//OA auto Edge4 = BRepBuilderAPI_MakeEdge(Point4, Point1);
			//OA TopoDS_Wire RecSecWire = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//OA TopoDS_Shape RecSecFace = BRepBuilderAPI_MakeFace(RecSecWire);

			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(RecSecFace);
			//OA _aisshape = aisshape;
			//OA BaseshowBeamSec(aisshape);
			QString name = mPBeam::getInstance()->_nameParaListPair.key(getPara());;

			QList<QString> NAMElist = beamView::_nameAisPair.keys();
			if (!NAMElist.contains(name))
			{
			/*	emit AddSecSignal(name);
				emit showParaOnPanel(_para);*/
			}
			else
			{
				beamView::_nameAisPair.remove(name);
				mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());
			}

			//OA beamView::_nameAisPair.insert(name, aisshape);
			//OA beamView::_secNameAisPair.insert(name, aisshape);
			//OA getDataFromBeam(name, RecSecFace);
			//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();
			//OA GProp_GProps System;
			//OA BRepGProp::SurfaceProperties(RecSecFace, System);
			//OA auto CentreOfMass = System.CentreOfMass();
			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(CentreOfMass.X(), CentreOfMass.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//_geoWindow->getAisContext()->Display(shearMarker, Standard_False);
			//OA beamView::_nameAisPair.insert(name, shearMarker);
			//OA _shearMarker = shearMarker;

			//OA Handle(AIS_LengthDimension) a14LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge4), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an14Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString aValue = a14LenDim->GetValue();
			//OA TCollection_AsciiString atext = "DIM1 = ";
			//OA an14Aspect->SetValueStringFormat(atext + aValue);
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
			//OA _a14LenDim = a14LenDim;
			//OA beamView::_nameAisPair.insert(name, a14LenDim);

			//OA Handle(AIS_LengthDimension) a12LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge1), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an12Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValue = a12LenDim->GetValue();
			//OA TCollection_AsciiString btext = "DIM2 = ";
			//OA an12Aspect->SetValueStringFormat(btext + bValue);
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
			//OA _a12LenDim = a12LenDim;
			//OA beamView::_nameAisPair.insert(name, a12LenDim);

			//求抗扭截面系数
			double t, bi, Ci;
			if (a < b)
			{
				t = a;
				bi = b;
			}
			else
			{
				bi = a;
				t = b;
			}
			if (t / bi < 0.1)
			{
				Ci = 1.0 / 3.0;
			}
			else
			{
				Ci = (1.0 / 3.0) * (1.0 - 0.630 *t / bi + 0.052*pow((t / bi), 5));

			}
			double It = Ci * bi*pow(t, 3);
			//double It1 = (bi*pow(t, 3) / 16.0)*(16.0 / 3.0 - (3.36*t / bi)*(1 - pow(t, 4) / 12.0 / pow(bi, 4)));
			_data.push_back(It);
			//OA _data.push_back(CentreOfMass.Y());
			//OA _data.push_back(CentreOfMass.X());
			_data.push_back(5.0 / 6.0);
			_data.push_back(5.0 / 6.0);
			//emit showDataOnPanel(_data);
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
			QList<QVector2D> PointCoor;
			QVector2D C, D, E, F;
			C.setX(a / 2.0); C.setY(b / 2.0);
			D.setX(a / 2.0); D.setY(b / 2.0);
			E.setX(a / 2.0); E.setY(b / 2.0);
			F.setX(a / 2.0); F.setY(b / 2.0);
			PointCoor.push_back(C);
			PointCoor.push_back(D);
			PointCoor.push_back(E);
			PointCoor.push_back(F);
			mPBeam::getInstance()->_namePointListPair.insert(name, PointCoor);
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
		}
		

		
	}
}

