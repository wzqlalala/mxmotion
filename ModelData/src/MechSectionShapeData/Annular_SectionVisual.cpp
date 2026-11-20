#include "Annular_SectionVisual.h"
//#include "gp_Pnt.hxx"
//#include "BRepBuilderAPI_MakeEdge.hxx"
//#include "BRepBuilderAPI_MakeFace.hxx"
//#include "BRepBuilderAPI_MakeWire.hxx"
//#include "TopoDS_Face.hxx"
//#include "TopoDS_Wire.hxx"
//#include "gp_Circ.hxx"
//#include "gp_Torus.hxx"
//#include "BRepOffsetAPI_MakeOffset.hxx"
//#include "BRepAlgoAPI_Cut.hxx"

//#include <Prs3d_Drawer.hxx>
//#include <AIS_InteractiveContext.hxx>
//#include <AIS_LengthDimension.hxx>
// #include "MxBeam/GeoWidget.h"
//#include "GC_MakePlane.hxx"
// #include "MXBeamData/MXBeamDataBase.h"
//#include "AIS_RadiusDimension.hxx"
#include "Sample2D_Markers.h"
//#include <BRepGProp.hxx>
//#include <GProp_GProps.hxx>

//MDataProperty
//#include "mPBeam.h"
#include <mGlobalSignals.h>

//#include "GeoWidget.h"
#include "beamView.h"
#include "mPBeam.h"

// ;
namespace MechSectionShapeData
{
#define PI 3.1415926535898
	Annular_SectionVisual::Annular_SectionVisual(QString name, int id)
		: mSectionShapeBase(Sec_Ann,name,id)
	{
		_para.push_back(_R1);
		_para.push_back(_R2);
	}

	Annular_SectionVisual::~Annular_SectionVisual()
	{

	}
	Annular_SectionVisual* Annular_SectionVisual::clone() const
	{


		Annular_SectionVisual* newSec = new Annular_SectionVisual(_sectionShapeName, _id);

		newSec->_R1 = _R1;
		newSec->_R2 = _R2;


		newSec->_shearMarker = _shearMarker;
		newSec->_a14LenDim = _a14LenDim;
		newSec->_aLenDim = _aLenDim;
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
	void Annular_SectionVisual::CreateAnuSec()
	{
		try
		{
			//OA gp_Dir dir(0, 0, 1);
			//OA gp_Pnt point(0, 0, 0); gp_Pnt point3(0, _R2, 0);
			//OA gp_Ax2 ax(point, dir);
			//OA gp_Circ cir1(ax, _R1);
			//OA auto edge1 = BRepBuilderAPI_MakeEdge(cir1);
			//OA gp_Circ cir2(ax, _R2);
			//OA auto edge2 = BRepBuilderAPI_MakeEdge(cir2);
			//OA auto AnuSecWire1 = BRepBuilderAPI_MakeWire(edge1);
			//OA auto AnuSecWire2 = BRepBuilderAPI_MakeWire(edge2);
			//OA TopoDS_Shape face1 = BRepBuilderAPI_MakeFace(AnuSecWire1);

			//OA TopoDS_Shape face2 = BRepBuilderAPI_MakeFace(AnuSecWire2);
			//OA TopoDS_Shape AnuSecFace = BRepAlgoAPI_Cut(face1, face2);

			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(AnuSecFace);
			//OA _aisshape = aisshape;
			//OA BaseshowBeamSec(aisshape);
			QString name = mPBeam::getInstance()->setDefaultNameForBeam(Sec_Ann);
			/*emit AddSecSignal(name);
			emit showParaOnPanel(_para, name);*/
			//OA beamView::_nameAisPair.insert(name, aisshape);
			//OA beamView::_secNameAisPair.insert(name, aisshape);
			mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());

			//OA getDataFromBeam(name, AnuSecFace);
			//OA GProp_GProps System;
			//OA BRepGProp::SurfaceProperties(AnuSecFace, System);
			//OA auto CentreOfMass = System.CentreOfMass();
			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(CentreOfMass.X(), CentreOfMass.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//_geoWindow->getAisContext()->Display(shearMarker, Standard_False);
			//OA beamView::_nameAisPair.insert(name, shearMarker);
			//OA _shearMarker = shearMarker;

			//OA GC_MakePlane aMkPlane(point, dir);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();

			//OA Handle(AIS_RadiusDimension) a14LenDim = new AIS_RadiusDimension(cir1);
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

			//OA Handle(AIS_RadiusDimension) aLenDim = new AIS_RadiusDimension(cir2, point3);
			//OA Handle(Prs3d_DimensionAspect) anAspect = new Prs3d_DimensionAspect();
			//OA aLenDim->SetDisplaySpecialSymbol(PrsDim_DisplaySpecialSymbol_No);

			//OA TCollection_AsciiString bValue = aLenDim->GetValue();
			//OA TCollection_AsciiString b = "DIM2 = ";
			//OA anAspect->SetValueStringFormat(b + bValue);
			//OA anAspect->MakeArrows3d(Standard_False);
			//OA anAspect->MakeText3d(Standard_False);
			//OA anAspect->TextAspect()->SetHeight(0.3);
			//OA anAspect->ArrowAspect()->SetLength(0.8);
			//OA anAspect->MakeTextShaded(Standard_False);
			//OA anAspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA anAspect->MakeUnitsDisplayed(Standard_False);
			//OA anAspect->SetArrowTailSize(0.3);
			//OA anAspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);

			//anAspect->TextAspect()->SetAngle(10);
			//OA aLenDim->SetDimensionAspect(anAspect);
			//_geoWindow->getAisContext()->Display(aLenDim, true);
			//OA beamView::_nameAisPair.insert(name, aLenDim);
			//OA _aLenDim = aLenDim;

			//扭转常数计算
			double It = PI * (pow(_R1 * 2, 4) - pow(_R2 * 2, 4)) / 32;
			_data.push_back(It);
			//OA _data.push_back(CentreOfMass.Y());
			//OA _data.push_back(CentreOfMass.X());
			_data.push_back(1.0 / 2.0);
			_data.push_back(1.0 / 2.0);
			//emit showDataOnPanel(_data);
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
		}
		
	}

	void Annular_SectionVisual::ReBuildAnnSec(QList<double> para)
	{
		try
		{
			_R1 = para.at(1);
			_R2 = para.at(2);
			//_geoWindow->getAisContext()->EraseAll(true);
			para.removeFirst();
			_para = para;

			//OA gp_Dir dir(0, 0, 1);
			//OA gp_Pnt point(0, 0, 0); gp_Pnt point3(0, _R2, 0);
			//OA gp_Ax2 ax(point, dir);
			//OA gp_Circ cir1(ax, _R1);
			//OA auto edge1 = BRepBuilderAPI_MakeEdge(cir1);
			//OA gp_Circ cir2(ax, _R2);
			//OA auto edge2 = BRepBuilderAPI_MakeEdge(cir2);
			//OA auto AnuSecWire1 = BRepBuilderAPI_MakeWire(edge1);
			//OA auto AnuSecWire2 = BRepBuilderAPI_MakeWire(edge2);
			//OA TopoDS_Shape face1 = BRepBuilderAPI_MakeFace(AnuSecWire1);

			//OA TopoDS_Shape face2 = BRepBuilderAPI_MakeFace(AnuSecWire2);
			//OA TopoDS_Shape AnuSecFace = BRepAlgoAPI_Cut(face1, face2);

			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(AnuSecFace);
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



			//OA getDataFromBeam(name, AnuSecFace);
			//OA GProp_GProps System;
			//OA BRepGProp::SurfaceProperties(AnuSecFace, System);
			//OA auto CentreOfMass = System.CentreOfMass();
			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(CentreOfMass.X(), CentreOfMass.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//_geoWindow->getAisContext()->Display(shearMarker, Standard_False);
			//OA beamView::_nameAisPair.insert(name, shearMarker);
			//OA _shearMarker = shearMarker;

			//OA GC_MakePlane aMkPlane(point, dir);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();

			//OA Handle(AIS_RadiusDimension) a14LenDim = new AIS_RadiusDimension(cir1);
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

			//OA Handle(AIS_RadiusDimension) aLenDim = new AIS_RadiusDimension(cir2, point3);
			//OA Handle(Prs3d_DimensionAspect) anAspect = new Prs3d_DimensionAspect();
			//OA aLenDim->SetDisplaySpecialSymbol(PrsDim_DisplaySpecialSymbol_No);

			//OA TCollection_AsciiString bValue = aLenDim->GetValue();
			//OA TCollection_AsciiString b = "DIM2 = ";
			//OA anAspect->SetValueStringFormat(b + bValue);
			//OA anAspect->MakeArrows3d(Standard_False);
			//OA anAspect->MakeText3d(Standard_False);
			//OA anAspect->TextAspect()->SetHeight(0.3);
			//OA anAspect->ArrowAspect()->SetLength(0.8);
			//OA anAspect->MakeTextShaded(Standard_False);
			//OA anAspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA anAspect->MakeUnitsDisplayed(Standard_False);
			//OA anAspect->SetArrowTailSize(0.3);
			//OA anAspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA anAspect->SetTextVerticalPosition(Prs3d_DTVP_Center);
			//OA anAspect->TextAspect()->SetAngle(10);
			//OA aLenDim->SetDimensionAspect(anAspect);
			//_geoWindow->getAisContext()->Display(aLenDim, true);
			//OA beamView::_nameAisPair.insert(name, aLenDim);
			//OA _aLenDim = aLenDim;

			//扭转常数计算
			double It = PI * (pow(_R1 * 2, 4) - pow(_R2 * 2, 4)) / 32;
			_data.push_back(It);
			//OA _data.push_back(CentreOfMass.Y());
			//OA _data.push_back(CentreOfMass.X());

			_data.push_back(1.0 / 2.0);
			_data.push_back(1.0 / 2.0);
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
			//emit showDataOnPanel(_data);
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
		}
		
	}
}

