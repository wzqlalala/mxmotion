
#include "mCustomSectionShape.h"

//OA #include "AIS_Shape.hxx"
//OA #include <BRepBndLib.hxx>
//OA #include <Prs3d_Drawer.hxx>
//OA #include <AIS_InteractiveContext.hxx>
//OA #include <AIS_LengthDimension.hxx>
//OA #include "GC_MakePlane.hxx"
//OA #include <BRepBndLib.hxx>
//OA #include "Sample2D_Markers.h"
//OA #include <BRepGProp.hxx>
//OA #include <GProp_GProps.hxx>

//#include "beamView.h"
//OA #include <GProp_PrincipalProps.hxx>
#include "mCustomSectionShape.h"
using namespace MechSectionShapeData;
namespace MechSectionShapeData
{
    mCustomSectionShape::mCustomSectionShape(const QString& name, int id) :  mSectionShapeBase(Sec_None,name, id)
    {
        if (name.isEmpty())
        {
            _sectionShapeName = QString("%1_%2").arg(getSectionShapeTypeStr()).arg(_id);
        }
        _para.clear();
    }
    mCustomSectionShape::~mCustomSectionShape()
    {

    }
    mCustomSectionShape* mCustomSectionShape::clone() const
    {
		mCustomSectionShape* newI1 = new mCustomSectionShape(_sectionShapeName, _id);
		newI1->_customPara = _customPara;
		newI1->_para = _para;
		newI1->_data = _data;
		newI1->area = area;
		newI1->Iz = Iz;
		newI1->Iy = Iy;
		newI1->J = J;
		newI1->Iyz = Iyz;
		newI1->K1 = K1;
		newI1->K2 = K2;

		return newI1;
    }

    const QList<QString> mCustomSectionShape::getCustomPara()
    {
        return _customPara;
    }

    void mCustomSectionShape::setCustomPara(QList<QString>& para)
    {
        _customPara = para;
    }

    void mCustomSectionShape::CreateSec()
    {
        _customPara.reserve(24);
        for (int i = 0; i < 24; i++)
        {
            _customPara.append("");
        }
    }

} // namespace MechSectionShapeData