//#pragma once
//
//#include <QObject>
//
//#include "TopoDS_Shape.hxx"
//#include <AIS_Shape.hxx>
//
//namespace MxBeam
//{
//	class MainWindow;
//	class GeoWidget;
//}
//namespace MechSectionShapeData
//{
//#define PI 3.1415926535898
//	class  BeamSectionVisualBase : public QObject
//	{
//		Q_OBJECT
//
//	public:
//		BeamSectionVisualBase(QWidget*, MxBeam::GeoWidget*);
//		~BeamSectionVisualBase();
//		void BaseshowBeamSec(Handle(AIS_Shape));//显示截面
//		void getDataFromBeam(QString name , TopoDS_Shape beamShape);
//		QList<double> _data;
//		signals:
//			void setGridValue();
//			void showOriginalAis();
//			void AddSecSignal(QString);
//			void showParaOnPanel(QList<double>);
//			void showParaOnPanel(QList<double>,QString);
//			void showDataOnPanel(QList<double>);
//
//	protected:
//		double area, Iz, Iy, J, Iyz, K1, K2;
//	private:
//		QWidget* _mainWindow{};
//		MxBeam::GeoWidget* _geoWindow{};
//	 
//	
//	};
//}
//
