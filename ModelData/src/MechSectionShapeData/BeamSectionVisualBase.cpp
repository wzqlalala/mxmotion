//#include "BeamSectionVisualBase.h"
//// #include "MxBeam/MainWindow.h"
// //#include "GeoWidget.h"
//#include "AIS_Shape.hxx"
//#include <BRepBndLib.hxx>
//#include <Prs3d_Drawer.hxx>
//#include <AIS_InteractiveContext.hxx>
//#include <AIS_LengthDimension.hxx>
//#include "GC_MakePlane.hxx"
//#include <BRepBndLib.hxx>
//#include "Sample2D_Markers.h"
//#include <BRepGProp.hxx>
//#include <GProp_GProps.hxx>
//#include "mPBeam.h"
////#include "beamView.h"
//#include <GProp_PrincipalProps.hxx>
//namespace MechSectionShapeData
//{
//	BeamSectionVisualBase::BeamSectionVisualBase(QWidget*m, MxBeam::GeoWidget*g)
//		: _mainWindow(m),_geoWindow(g)
//	{
//	     // connect(this, &showBeamSec, _geoWindow, &MxBeam::GeoWidget::slotShowBeamSec);
//		//connect(this,SIGNAL(showOriginalAis()), _geoWindow,SLOT(slotShowaisOriginTrihedron()));
//		//connect(this, SIGNAL(AddSecSignal(QString)), _mainWindow, SIGNAL(AddSecSig(QString)));
//		//connect(this, SIGNAL(showParaOnPanel(QList<double>)),_mainWindow, SIGNAL(showParaOnPanelSig(QList<double>)));
//		//connect(this, SIGNAL(showParaOnPanel(QList<double>,QString)), _mainWindow, SIGNAL(showParaOnPanelSig(QList<double>, QString)));
//		//connect(this, SIGNAL(showDataOnPanel(QList<double>)), _mainWindow, SIGNAL(showDataOnPanelSig(QList<double>)));
//		//connect(this, SIGNAL(setGridValue()), _geoWindow, SLOT(SetRectangularGridGraphicValues()));
//	}
//
//	BeamSectionVisualBase::~BeamSectionVisualBase()
//	{
//	}
//
//
//
//	void BeamSectionVisualBase::BaseshowBeamSec(Handle(AIS_Shape) aisshape)
//	{
//		//_geoWindow->_aisshape = aisshape;
//		aisshape->SetDisplayMode(AIS_Shaded);
//		aisshape->SetColor(Quantity_NOC_GREENYELLOW);
//		aisshape->SetTransparency(0.4);
//		auto shape=aisshape->Shape();
//		//_geoWindow->getAisContext()->Display(aisshape, false);
//		Bnd_Box bbox = Bnd_Box();
//		bbox.SetGap(1e-4);
//		BRepBndLib  brepbndlib;
//		brepbndlib.Add(shape, bbox, true);
//		//_geoWindow->getV3dView()->FitAll(bbox, 0.5, true);
//		//_geoWindow->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
//	
//		emit setGridValue();
//		emit showOriginalAis();
//	}
//
//	void BeamSectionVisualBase::getDataFromBeam(QString name, TopoDS_Shape beamShape)
//	{
//		GProp_GProps System;
//		BRepGProp::SurfaceProperties(beamShape, System);
//		auto CentreOfMass = System.CentreOfMass();
//
//		Handle(Sample2D_Markers) aMarker =
//			new Sample2D_Markers(CentreOfMass.X(), CentreOfMass.Y(), Aspect_TOM_O_PLUS, Quantity_NOC_BLACK);
//		
//		//_geoWindow->getAisContext()->Display(aMarker, Standard_False);
//		//beamView::_nameAisPair.insert(name, aMarker);
//
//		auto Area = System.Mass();
//		auto max = System.MatrixOfInertia();
//		auto ppS = System.PrincipalProperties();
//		
//		double Iyx = -max.Value(1, 2);
//		double Ixx = max.Value(1, 1);
//		double Iyy = max.Value(2, 2);
//
//		_data.push_back(Area);
//		_data.push_back(CentreOfMass.Y());
//		_data.push_back(CentreOfMass.X());
//		_data.push_back(Iyy);
//		_data.push_back(Ixx);
//		_data.push_back(Iyx);
//		
//		//double cc = max.Value(3, 3);
//		/*double ee = max.Value(1, 3);
//		double ff = max.Value(2, 1);
//		double gg = max.Value(2, 3);
//		double hh = max.Value(3, 1);
//		double ii = max.Value(3, 2);*/
//		
//	}
//}
//
