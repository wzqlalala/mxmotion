#include "L_SectionVisual.h"
//#include "gp_Pnt.hxx"
//#include "BRepBuilderAPI_MakeEdge.hxx"
//#include "BRepBuilderAPI_MakeFace.hxx"
//#include "BRepBuilderAPI_MakeWire.hxx"
//#include "TopoDS_Face.hxx"


//#include <Prs3d_Drawer.hxx>
//#include <AIS_InteractiveContext.hxx>
//#include <AIS_LengthDimension.hxx>
//#include "GC_MakePlane.hxx"
//#include <BRepBndLib.hxx>
#include "Sample2D_Markers.h"
//#include <BRepGProp.hxx>
//#include <GProp_GProps.hxx>

//MDataProperty
#include <mGlobalSignals.h>

#include "mPBeam.h"
//#include "GeoWidget.h"
//#include <TopoDS_Wire.hxx>
#include "beamView.h"
namespace MechSectionShapeData
{
	L_SectionVisual::L_SectionVisual(QString name, int id)
		:mSectionShapeBase(Sec_L,name,id)
	{
		_para.push_back(b);
		_para.push_back(a);
		_para.push_back(t1);
		_para.push_back(t2);
	
	}

	L_SectionVisual::~L_SectionVisual()
	{
	}
	L_SectionVisual* L_SectionVisual::clone() const
	{
		L_SectionVisual* newSec = new L_SectionVisual(_sectionShapeName, _id);
		newSec->DIM1 = DIM1;
		newSec->DIM2 = DIM2;
		newSec->DIM3 = DIM3;
		newSec->DIM4 = DIM4;
		newSec->a = a;
		newSec->b = b;
		newSec->t2 = t2;
		newSec->t1 = t1;
		newSec->_X = _X;
		newSec->_Y = _Y;
		newSec->_Z = _Z;

		newSec->_shearMarker = _shearMarker;
		newSec->_a16LenDim = _a16LenDim;
		newSec->_a12LenDim = _a12LenDim;
		newSec->_a23LenDim = _a23LenDim;
		newSec->_a34LenDim = _a34LenDim;
		newSec->_name = _name;
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
	void L_SectionVisual::CreateLSec()
	{
		try
		{
			//OA gp_Pnt Point1(_X, _Y, _Z); gp_Pnt Point2(_X, -_Y, _Z); gp_Pnt Point3(-_X, -_Y, _Z);
			//OA gp_Pnt Point4(-_X, -_Y + t1, _Z); gp_Pnt Point5(_X + t2, -_Y + t1, _Z); gp_Pnt Point6(_X + t2, _Y, _Z);
			//OA gp_Pnt Point16((_X + _X + t2) / 2, _Y + 0.5, _Z);
			//OA gp_Pnt Point12(_X - 0.5, 0, _Z);
			//OA gp_Pnt Point23(0, -_Y - 0.5, _Z);
			//OA gp_Pnt Point34(-_X + 0.5, (-2 * _Y + t1) / 2, _Z);
			//OA gp_Pnt shearCenter(_X + t2 / 2, -_Y + t1 / 2, _Z);
			//OA TopoDS_Edge Edge1 = BRepBuilderAPI_MakeEdge(Point1, Point2);
			//OA TopoDS_Edge Edge2 = BRepBuilderAPI_MakeEdge(Point2, Point3);
			//OA TopoDS_Edge Edge3 = BRepBuilderAPI_MakeEdge(Point3, Point4);
			//OA TopoDS_Edge Edge4 = BRepBuilderAPI_MakeEdge(Point4, Point5);
			//OA TopoDS_Edge Edge5 = BRepBuilderAPI_MakeEdge(Point5, Point6);
			//OA TopoDS_Edge Edge6 = BRepBuilderAPI_MakeEdge(Point6, Point1);
			//OA auto LSecWire1 = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//OA TopoDS_Wire LSecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6);
			//OA LSecWire1.Add(LSecWire2);
			//OA TopoDS_Shape LSecFace = BRepBuilderAPI_MakeFace(LSecWire1);
			QString name = mPBeam::getInstance()->setDefaultNameForBeam(Sec_L);
			mPBeam::getInstance()->_nameParaListPair.insert(name, getPara());
			/*emit AddSecSignal(name);
			emit showParaOnPanel(_para, name);*/

			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(LSecFace);
			//OA beamView::_nameAisPair.insert(name, aisshape);
			//OA beamView::_secNameAisPair.insert(name, aisshape);
			//OA _aisshape = aisshape;
			//OA BaseshowBeamSec(aisshape);
			//OA getDataFromBeam(name, LSecFace);
			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(shearCenter.X(), shearCenter.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			//_geoWindow->getAisContext()->Display(shearMarker, Standard_False);
			//OA _shearMarker = shearMarker;
			//OA beamView::_nameAisPair.insert(name, shearMarker);
			//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();
			//OA Handle(AIS_LengthDimension) a16LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge6), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an16Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t2Value = a16LenDim->GetValue();
			//OA TCollection_AsciiString t2text = "DIM4 = ";
			//OA an16Aspect->SetValueStringFormat(t2text + t2Value);
			//OA an16Aspect->ArrowAspect()->SetLength(0.2);
			//OA an16Aspect->MakeArrows3d(Standard_False);
			//OA an16Aspect->MakeText3d(Standard_False);
			//OA an16Aspect->TextAspect()->SetHeight(0.3);
			//OA an16Aspect->MakeTextShaded(Standard_True);
			//OA an16Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an16Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an16Aspect->SetArrowTailSize(0.3);
			//OA an16Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a16LenDim->SetTextPosition(Point16);
			//OA a16LenDim->SetDimensionAspect(an16Aspect);

		//	_geoWindow->getAisContext()->Display(a16LenDim, true);
			//OA _a16LenDim = a16LenDim;
			//OA beamView::_nameAisPair.insert(name, a16LenDim);

			//OA Handle(AIS_LengthDimension) a12LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge1), aPlane->Pln());
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
			//OA _a12LenDim = a12LenDim;
			//OA beamView::_nameAisPair.insert(name, a12LenDim);
			//OA Handle(AIS_LengthDimension) a23LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge2), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an23Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValue = a23LenDim->GetValue();
			//OA TCollection_AsciiString btext = "DIM1 = ";
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
			//OA _a23LenDim = a23LenDim;
			//OA beamView::_nameAisPair.insert(name, a23LenDim);
			//OA Handle(AIS_LengthDimension) a34LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge3), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an34Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t1Value = a34LenDim->GetValue();
			//OA TCollection_AsciiString t1text = "DIM3 = ";
			//OA an34Aspect->SetValueStringFormat(t1text + t1Value);
			//OA an34Aspect->ArrowAspect()->SetLength(0.2);
			//OA an34Aspect->MakeArrows3d(Standard_False);
			//OA an34Aspect->MakeText3d(Standard_False);
			//OA an34Aspect->TextAspect()->SetHeight(0.3);
			//OA an34Aspect->MakeTextShaded(Standard_True);
			//OA an34Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an34Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an34Aspect->SetArrowTailSize(0.3);
			//OA an34Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a34LenDim->SetTextPosition(Point34);
			//OA a34LenDim->SetDimensionAspect(an34Aspect);
			//_geoWindow->getAisContext()->Display(a34LenDim, true);
			//OA _a34LenDim = a34LenDim;
			//OA beamView::_nameAisPair.insert(name, a34LenDim);

			//求抗扭截面系数
			double ti1, bi1, Ci1 = 1.0 / 3.0;

			bi1 = b - 0.5*t2;
			ti1 = t1;
			double It1 = Ci1 * bi1*pow(ti1, 3);

			double ti2, bi2, Ci2 = 1.0 / 3.0;

			bi2 = a - 0.5* t1;
			ti2 = t2;
			double It2 = Ci2 * bi2*pow(ti2, 3);

			double It = It1 + It2;

			_data.push_back(It);
			//OA _data.push_back(shearCenter.Y());
			//OA _data.push_back(shearCenter.X());
			double SFY = (a - t1)*t2 / _data.at(0);
			double SFZ = (b - t2)*t1 / _data.at(0);
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

	void L_SectionVisual::ReBuildLSec(QList<double> para)
	{
		try
		{
			b = para.at(1);
			a = para.at(2);
			t1 = para.at(3);
			t2 = para.at(4);
			/*_geoWindow->getAisContext()->EraseAll(true);*/
			para.removeFirst();
			_para = para;

			_X = _X - (b - 6) / 2;
			_Y = _Y + (a - 6) / 2;
			//OA gp_Pnt Point1(_X, _Y, _Z); gp_Pnt Point2(_X, -_Y, _Z); gp_Pnt Point3(-_X, -_Y, _Z);
			//OA gp_Pnt Point4(-_X, -_Y + t1, _Z); gp_Pnt Point5(_X + t2, -_Y + t1, _Z); gp_Pnt Point6(_X + t2, _Y, _Z);
			//OA gp_Pnt Point16((_X + _X + t2) / 2, _Y + 0.5, _Z);
			//OA gp_Pnt Point12(_X - 0.5, 0, _Z);
			//OA gp_Pnt Point23(0, -_Y - 0.5, _Z);
			//OA gp_Pnt Point34(-_X + 0.5, (-2 * _Y + t1) / 2, _Z);
			//OA gp_Pnt shearCenter(_X + t2 / 2, -_Y + t1 / 2, _Z);
			//OA TopoDS_Edge Edge1 = BRepBuilderAPI_MakeEdge(Point1, Point2);
			//OA TopoDS_Edge Edge2 = BRepBuilderAPI_MakeEdge(Point2, Point3);
			//OA TopoDS_Edge Edge3 = BRepBuilderAPI_MakeEdge(Point3, Point4);
			//OA TopoDS_Edge Edge4 = BRepBuilderAPI_MakeEdge(Point4, Point5);
			//OA TopoDS_Edge Edge5 = BRepBuilderAPI_MakeEdge(Point5, Point6);
			//OA TopoDS_Edge Edge6 = BRepBuilderAPI_MakeEdge(Point6, Point1);
			//OA auto LSecWire1 = BRepBuilderAPI_MakeWire(Edge1, Edge2, Edge3, Edge4);
			//OA TopoDS_Wire LSecWire2 = BRepBuilderAPI_MakeWire(Edge5, Edge6);
			//OA LSecWire1.Add(LSecWire2);
			//OA TopoDS_Shape LSecFace = BRepBuilderAPI_MakeFace(LSecWire1);
			QString name = mPBeam::getInstance()->_nameParaListPair.key(getPara());
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

			//OA Handle(AIS_Shape) aisshape = new AIS_Shape(LSecFace);
			//OA _aisshape = aisshape;
			//OA beamView::_nameAisPair.insert(name, aisshape);
			//OA beamView::_secNameAisPair.insert(name, aisshape);
			//OA BaseshowBeamSec(aisshape);


			//OA getDataFromBeam(name, LSecFace);
			//OA Handle(Sample2D_Markers) shearMarker =
				//OA new Sample2D_Markers(shearCenter.X(), shearCenter.Y(), Aspect_TOM_O_X, Quantity_NOC_YELLOW);
			/*_geoWindow->getAisContext()->Display(shearMarker, Standard_False);*/

			//OA beamView::_nameAisPair.insert(name, shearMarker);
			//OA _shearMarker = shearMarker;

			//OA GC_MakePlane aMkPlane(Point1, Point2, Point3);
			//OA Handle(Geom_Plane) aPlane = aMkPlane.Value();
			//OA Handle(AIS_LengthDimension) a16LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge6), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an16Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t2Value = a16LenDim->GetValue();
			//OA TCollection_AsciiString t2text = "DIM4 = ";
			//OA an16Aspect->SetValueStringFormat(t2text + t2Value);
			//OA an16Aspect->ArrowAspect()->SetLength(0.2);
			//OA an16Aspect->MakeArrows3d(Standard_False);
			//OA an16Aspect->MakeText3d(Standard_False);
			//OA an16Aspect->TextAspect()->SetHeight(0.3);
			//OA an16Aspect->MakeTextShaded(Standard_True);
			//OA an16Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an16Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an16Aspect->SetArrowTailSize(0.3);
			//OA an16Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a16LenDim->SetTextPosition(Point16);
			//OA a16LenDim->SetDimensionAspect(an16Aspect);

			//_geoWindow->getAisContext()->Display(a16LenDim, true);
			//OA _a16LenDim = a16LenDim;
			//OA beamView::_nameAisPair.insert(name, a16LenDim);

			//OA Handle(AIS_LengthDimension) a12LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge1), aPlane->Pln());
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
			//OA _a12LenDim = a12LenDim;
			//OA beamView::_nameAisPair.insert(name, a12LenDim);
			//OA Handle(AIS_LengthDimension) a23LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge2), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an23Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString bValue = a23LenDim->GetValue();
			//OA TCollection_AsciiString btext = "DIM1 = ";
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
			//OA _a23LenDim = a23LenDim;
			//OA beamView::_nameAisPair.insert(name, a23LenDim);
			//OA Handle(AIS_LengthDimension) a34LenDim = new AIS_LengthDimension(TopoDS::Edge(Edge3), aPlane->Pln());
			//OA Handle(Prs3d_DimensionAspect) an34Aspect = new Prs3d_DimensionAspect();
			//OA TCollection_AsciiString t1Value = a34LenDim->GetValue();
			//OA TCollection_AsciiString t1text = "DIM3 = ";
			//OA an34Aspect->SetValueStringFormat(t1text + t1Value);
			//OA an34Aspect->ArrowAspect()->SetLength(0.2);
			//OA an34Aspect->MakeArrows3d(Standard_False);
			//OA an34Aspect->MakeText3d(Standard_False);
			//OA an34Aspect->TextAspect()->SetHeight(0.3);
			//OA an34Aspect->MakeTextShaded(Standard_True);
			//OA an34Aspect->SetCommonColor(Quantity_NOC_BLACK);
			//OA an34Aspect->MakeUnitsDisplayed(Standard_False);
			//OA an34Aspect->SetArrowTailSize(0.3);
			//OA an34Aspect->SetTextHorizontalPosition(Prs3d_DTHP_Center);
			//OA a34LenDim->SetTextPosition(Point34);
			//OA a34LenDim->SetDimensionAspect(an34Aspect);
			//_geoWindow->getAisContext()->Display(a34LenDim, true);
			//OA _a34LenDim = a34LenDim;
			//OA beamView::_nameAisPair.insert(name, a34LenDim);

			//求抗扭截面系数
			double ti1, bi1, Ci1 = 1.0 / 3.0;

			bi1 = b - 0.5*t2;
			ti1 = t1;
			double It1 = Ci1 * bi1*pow(ti1, 3);

			double ti2, bi2, Ci2 = 1.0 / 3.0;

			bi2 = a - 0.5* t1;
			ti2 = t2;
			double It2 = Ci2 * bi2*pow(ti2, 3);

			double It = It1 + It2;
			//double Ita = It1 + It2;
			//double ti3, bi3, Ci3 = 1.0 / 3.0;

			//bi3 = b;
			//ti3 = t1;
			//double It3 = Ci3 * bi3*pow(ti3, 3);
			//double ti4, bi4, Ci4 = 1.0 / 3.0;
			//bi4 = a - t1;
			//ti4 = t2;
			//double It4 = Ci4 * bi4*pow(ti4, 3);
			//double Itb = It3 + It4;
			//double It = (Ita + Itb) / 2.0;


			_data.push_back(It);
			//OA _data.push_back(shearCenter.Y());
			//OA _data.push_back(shearCenter.X());
			double SFY = (a - t1)*t2 / _data.at(0);
			double SFZ = (b - t2)*t1 / _data.at(0);
			_data.push_back(SFY);
			_data.push_back(SFZ);
			//emit showDataOnPanel(_data);
			mPBeam::getInstance()->_nameDataListPair.insert(name, _data);
		}
		catch (const std::exception& e)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, "截面创建失败！", true);
		}
		
		

	}

	void L_SectionVisual::initData()
	{
		DIM1 = 6, DIM2 = 6, DIM3 = 0.5, DIM4 = 1;

		a = 6;
		b = 6;
		t1 = 0.5;
		t2 = 1;
		_Y = a / 2;
		_X = -b / 2;
		_Z = 0;
	}
	
}

