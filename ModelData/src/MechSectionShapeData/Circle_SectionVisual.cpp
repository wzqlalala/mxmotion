#include "Circle_SectionVisual.h"
//#include "gp_Pnt.hxx"
//#include "BRepBuilderAPI_MakeEdge.hxx"
//#include "BRepBuilderAPI_MakeFace.hxx"
//#include "BRepBuilderAPI_MakeWire.hxx"
//#include "TopoDS_Face.hxx"
//#include "TopoDS_Wire.hxx"
//#include "gp_Circ.hxx"

//#include <Prs3d_Drawer.hxx>
//#include <AIS_InteractiveContext.hxx>
//#include <AIS_LengthDimension.hxx>
// #include "MxBeam/GeoWidget.h"
//#include "GC_MakePlane.hxx"

//#include "AIS_RadiusDimension.hxx"
#include "mPBeam.h"
#include "Sample2D_Markers.h"
//#include <BRepGProp.hxx>
//#include <GProp_GProps.hxx>
#include <mGlobalSignals.h>

//#include "gp_Cylinder.hxx"
//#include "GeoWidget.h"
#include "beamView.h"

namespace MechSectionShapeData
{
	Circle_SectionVisual::Circle_SectionVisual(QString name, int id)
		:mSectionShapeBase(Sec_Cir,name,id)
	{

		_para.push_back(_R);
	}

	Circle_SectionVisual::~Circle_SectionVisual()
	{
	}
	Circle_SectionVisual* Circle_SectionVisual::clone() const
	{
		Circle_SectionVisual* newSec = new Circle_SectionVisual(_sectionShapeName, _id);

		newSec->_R = _R;


		newSec->_shearMarker = _shearMarker;
		newSec->_a14LenDim = _a14LenDim;
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
	void Circle_SectionVisual::CreateCirSec()
	{
		try
		{
			//OA gp_Dir dir(0, 0, 1);
			//OA gp_Pnt point(0, 0, 0);
			//OA gp_Pnt point1(_R, 0, 0);
			//OA gp_Ax2 ax(point, dir);
			//OA gp_Circ cir(ax, _R);
			//OA auto edge = BRepBuilderAPI_MakeEdge(cir);
			//OA TopoDS_Wire CirSecWire = BRepBuilderAPI_MakeWire(edge);
			//OA TopoDS_Shape CirSecFace = BRepBuilderAPI_MakeFace(CirSecWire);
			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(CirSecFace);
			//OA _aisshape = aisshape;
			//OA BaseshowBeamSec(aisshape);
			QString name = mPBeam::getInstance()->setDefaultNameForBeam(Sec_Cir);
			/*emit AddSecSignal(name);
			emit showParaOnPanel(_para, name);*/

			//OA beamView::_nameAisPair.insert(name, aisshape);
			//OA beamView::_secNameAisPair.insert(name, aisshape);
			mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());

			//OA getDataFromBeam(name, CirSecFace);


			//OA GC_MakePlane aMkPlane(point, dir);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();
			//OA GProp_GProps System;
			//OA BRepGProp::SurfaceProperties(CirSecFace, System);
			//OA auto CentreOfMass = System.CentreOfMass();
			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(CentreOfMass.X(), CentreOfMass.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//_geoWindow->getAisContext()->Display(shearMarker, Standard_False);
			//OA beamView::_nameAisPair.insert(name, shearMarker);
			//OA _shearMarker = shearMarker;

			//OA Handle(AIS_RadiusDimension) a14LenDim = new AIS_RadiusDimension(cir);
			//OA Handle(Prs3d_DimensionAspect) an14Aspect = new Prs3d_DimensionAspect();
			//OA a14LenDim->SetDisplaySpecialSymbol(PrsDim_DisplaySpecialSymbol_No);

			//OA TCollection_AsciiString aValue = a14LenDim->GetValue();
			//OA TCollection_AsciiString a = "DIM1 = ";

			//OA an14Aspect->SetValueStringFormat(a + aValue);
			//OA an14Aspect->MakeArrows3d(Standard_False);
			//OA an14Aspect->MakeText3d(Standard_False);
			//OA an14Aspect->TextAspect()->SetHeight(0.3);
			//OA an14Aspect->ArrowAspect()->SetLength(0.8);
			//OA an14Aspect->MakeTextShaded(Standard_True);
			//OA an14Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an14Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an14Aspect->SetArrowTailSize(0.3);
			//OA an14Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a14LenDim->SetDimensionAspect(an14Aspect);
			//_geoWindow->getAisContext()->Display(a14LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a14LenDim);
			//OA _a14LenDim = a14LenDim;
			//扭转常数计算
			double It = 3.1415926535898 * pow(_R * 2, 4) / 32;
			_data.push_back(It);
			//OA _data.push_back(CentreOfMass.Y());
			//OA _data.push_back(CentreOfMass.X());
			_data.push_back(9.0 / 10.0);
			_data.push_back(9.0 / 10.0);
			//emit showDataOnPanel(_data);
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);

			QList<QVector2D> PointCoor;
			QVector2D C, D, E, F;
			C.setX(0); C.setY(_R);
			D.setX(_R); D.setY(0);
			E.setX(0); E.setY(-_R);
			F.setX(-_R); F.setY(0);
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

	void Circle_SectionVisual::ReBuildCirSec(QList<double> para)
	{
		try
		{
			//_geoWindow->getAisContext()->EraseAll(true);
			_R = para.at(1);
			para.removeFirst();
			_para = para;
		

			//OA gp_Dir dir(0, 0, 1);
			//OA gp_Pnt point(0, 0, 0);
			//OA gp_Pnt point1(_R, 0, 0);
			//OA gp_Ax2 ax(point, dir);
			//OA gp_Circ cir(ax, _R);
			//OA auto edge = BRepBuilderAPI_MakeEdge(cir);
			//OA TopoDS_Wire CirSecWire = BRepBuilderAPI_MakeWire(edge);
			//OA TopoDS_Shape CirSecFace = BRepBuilderAPI_MakeFace(CirSecWire);
			//OA gp_Cylinder a;
			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(CirSecFace);
			//OA _aisshape = aisshape;
			//OA BaseshowBeamSec(aisshape);
			QString name = mPBeam::getInstance()->_nameParaListPair.key(getPara());;

			QList<QString> NAMElist = beamView::_nameAisPair.keys();
			if (!NAMElist.contains(name))
			{
				/*emit AddSecSignal(name);
				emit showParaOnPanel(_para, name);*/
			}
			else
			{
				beamView::_nameAisPair.remove(name);
				mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());
			}
			//OA beamView::_nameAisPair.insert(name, aisshape);
			//OA beamView::_secNameAisPair.insert(name, aisshape);
			//OA getDataFromBeam(name, CirSecFace);
			//OA GC_MakePlane aMkPlane(point, dir);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();
			//OA GProp_GProps System;
			//OA BRepGProp::SurfaceProperties(CirSecFace, System);
			//OA auto CentreOfMass = System.CentreOfMass();
			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(CentreOfMass.X(), CentreOfMass.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//_geoWindow->getAisContext()->Display(shearMarker, Standard_False);
			//OA _shearMarker = shearMarker;
			//OA beamView::_nameAisPair.insert(name, shearMarker);
			//OA Handle(AIS_RadiusDimension) a14LenDim = new AIS_RadiusDimension(cir);
			//OA Handle(Prs3d_DimensionAspect) an14Aspect = new Prs3d_DimensionAspect();
			//OA a14LenDim->SetDisplaySpecialSymbol(PrsDim_DisplaySpecialSymbol_No);

			//OA TCollection_AsciiString bValue = a14LenDim->GetValue();
			//OA TCollection_AsciiString b = "DIM1 = ";

			//OA an14Aspect->SetValueStringFormat(b + bValue);

			//OA an14Aspect->MakeArrows3d(Standard_False);
			//OA an14Aspect->MakeText3d(Standard_False);
			//OA an14Aspect->TextAspect()->SetHeight(0.3);
			//OA an14Aspect->ArrowAspect()->SetLength(0.8);
			//OA an14Aspect->MakeTextShaded(Standard_True);
			//OA an14Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an14Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an14Aspect->SetArrowTailSize(0.3);
			//OA an14Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a14LenDim->SetDimensionAspect(an14Aspect);
			//_geoWindow->getAisContext()->Display(a14LenDim, true);
			//OA beamView::_nameAisPair.insert(name, a14LenDim);
			//OA _a14LenDim = a14LenDim;
			//扭转常数计算
			double It = 3.1415926535898 * pow(_R * 2, 4) / 32;
			_data.push_back(It);
			//OA _data.push_back(CentreOfMass.Y());
			//OA _data.push_back(CentreOfMass.X());
			_data.push_back(9.0 / 10.0);
			_data.push_back(9.0 / 10.0);
			//emit showDataOnPanel(_data);
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
			QList<QVector2D> PointCoor;
			QVector2D C, D, E, F;
			C.setX(0); C.setY(_R);
			D.setX(_R); D.setY(0);
			E.setX(0); E.setY(-_R);
			F.setX(-_R); F.setY(0);
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

