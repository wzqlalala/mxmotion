
#include "mSectionShapeBase.h"

//#include "AIS_Shape.hxx"
//#include <BRepBndLib.hxx>
//#include <Prs3d_Drawer.hxx>
//#include <AIS_InteractiveContext.hxx>
//#include <AIS_LengthDimension.hxx>
//#include "GC_MakePlane.hxx"
//#include <BRepBndLib.hxx>
#include "Sample2D_Markers.h"
//#include <BRepGProp.hxx>
//#include <GProp_GProps.hxx>

//#include "beamView.h"
//OA #include <GProp_PrincipalProps.hxx>
using namespace MechSectionShapeData;
namespace MechSectionShapeData
{
    int mSectionShapeBase:: _sectionShapeId = 1;

    mSectionShapeBase::mSectionShapeBase(MxFunctions::SecType type, const QString& name, int id) :_secType(type), _sectionShapeName(name), _id(id)
    {
        if (name.isEmpty())
        {
            _sectionShapeName = QString("%1_%2").arg(getSectionShapeTypeStr()).arg(_id);
        }
    }
    mSectionShapeBase::~mSectionShapeBase()
    {

    }
    mSectionShapeBase* mSectionShapeBase::clone() const
    {
        return new mSectionShapeBase(*this);
    }
    mSectionShapeBase* mSectionShapeBase::copySectionShape(const int& id, const QString& name)
    {
        mSectionShapeBase* section = new mSectionShapeBase(getSectionShapeType(), name, id);
        return section;
    }
    void mSectionShapeBase::setSectionShapeId(int id)
    {
        _id = id;
    }

    int mSectionShapeBase::getSectionShapeId()
    {
        return _id;
    }

    MxFunctions::SecType  mSectionShapeBase::getSectionShapeType() const
    {
        return _secType;
    }

    void mSectionShapeBase::setSectionShapeType(SecType type)
    {
        _secType = type;
    }

    void mSectionShapeBase::setSectionShapeName(const QString& name)
    {
        _sectionShapeName = name;
    }

    QString mSectionShapeBase::getSectionShapeName()
    {
        return _sectionShapeName;
    }

    QString mSectionShapeBase::getSectionShapeTypeStr()
    {
        switch (_secType)
        {
        case MxFunctions::Sec_H:
            return QString("H");
        case MxFunctions::Sec_L:
            return QString("L");
        case MxFunctions::Sec_T:
            return QString("T");
        case MxFunctions::Sec_Box:
            return QString("盒");
        case MxFunctions::Sec_Rec:
            return QString("矩形");
        case MxFunctions::Sec_Cir:
            return QString("圆");
        case MxFunctions::Sec_Ann:
            return QString("圆环");
        case MxFunctions::Sec_I:
            return QString("I");
        case MxFunctions::Sec_Channel:
            return QString("槽");
        case MxFunctions::Sec_Channel1:
            return QString("槽1");
        case MxFunctions::Sec_Channel2:
            return QString("槽2");
        case MxFunctions::Sec_I1:
            return QString("I1");
        case MxFunctions::Sec_T1:
            return QString("T1");
        case MxFunctions::Sec_Box1:
            return QString("盒1");
        case MxFunctions::Sec_T2:
            return QString("T2");
        case MxFunctions::Sec_Cross:
            return QString("十字");
        case MxFunctions::Sec_Hat:
            return QString("帽形");
        case MxFunctions::Sec_Z:
            return QString("Z");
        default:
            return QString("Custom");
        }
    }



    const QList<double>& mSectionShapeBase::getData()
    {
        return _data;
    }
   
    void mSectionShapeBase::clearData()
    {
        _data.clear();
    }
    const QList<double> mSectionShapeBase::getPara()
    {
        QList<double> _paraWithType = _para;   // 整体拷贝
        _paraWithType.prepend(double(_secType));
        return _paraWithType;
    }
    const QList<double> mSectionShapeBase::getParaNoType()
    {      
        return _para;
    }
    void mSectionShapeBase::setPara(QList<double>& para)
    {
        _para = para;
    }
    //OA void mSectionShapeBase::BaseshowBeamSec(Handle(AIS_Shape) aisshape)
    //OA dormant
//{
        //_geoWindow->_aisshape = aisshape;
        //OA aisshape->SetDisplayMode(AIS_Shaded);
        //OA aisshape->SetColor(Quantity_Color(0.046, 0.523, 0.896, Quantity_TOC_RGB));
        //OA aisshape->SetTransparency(0.4);
        //OA auto shape = aisshape->Shape();
        //_geoWindow->getAisContext()->Display(aisshape, false);
        //OA Bnd_Box bbox = Bnd_Box();
        //OA bbox.SetGap(1e-4);
        //OA BRepBndLib  brepbndlib;
        //OA brepbndlib.Add(shape, bbox, true);
        //OA Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
        //OA bbox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
        //OA return;
        //_geoWindow->getV3dView()->FitAll(bbox, 0.5, true);
        //_geoWindow->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));

//       /* emit setGridValue();
//        emit showOriginalAis();*/
//    }
    //OA void mSectionShapeBase::getDataFromBeam(QString name, TopoDS_Shape beamShape)
    //OA dormant
//{
        //OA GProp_GProps System;
        //OA BRepGProp::SurfaceProperties(beamShape, System);
        //OA auto CentreOfMass = System.CentreOfMass();

        //OA Handle(Sample2D_Markers) aMarker =
            //OA new Sample2D_Markers(CentreOfMass.X(), CentreOfMass.Y(), Aspect_TOM_O_PLUS, Quantity_NOC_BLACK);

        //_geoWindow->getAisContext()->Display(aMarker, Standard_False);
        //beamView::_nameAisPair.insert(name, aMarker);

        //OA auto Area = System.Mass();
        //OA auto max = System.MatrixOfInertia();
        //OA auto ppS = System.PrincipalProperties();

        //OA double Iyx = -max.Value(1, 2);
        //OA double Ixx = max.Value(1, 1);
        //OA double Iyy = max.Value(2, 2);

        //OA _data.push_back(Area);
        //OA _data.push_back(CentreOfMass.Y());
        //OA _data.push_back(CentreOfMass.X());
//        _data.push_back(Iyy);
//        _data.push_back(Ixx);
//        _data.push_back(Iyx);

        //double cc = max.Value(3, 3);
//        /*double ee = max.Value(1, 3);
//        double ff = max.Value(2, 1);
//        double gg = max.Value(2, 3);
//        double hh = max.Value(3, 1);
//        double ii = max.Value(3, 2);*/

    }

    void mSectionShapeBase::setNsm(QString nsm)
    {
        _nsm = nsm;
    }

    QString mSectionShapeBase::getNsm()
    {
        return _nsm;
    }
 
   
//OA } // namespace MechSectionShapeData