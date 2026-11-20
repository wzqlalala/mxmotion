#pragma once
#include "MXSectionShapeData_global.h"
#include <QString>
#include <vector>
#include "mPhysicalFieldEnum.h"

//#include "TopoDS_Shape.hxx"
//#include <AIS_Shape.hxx>
#include <QList>
//#include <AIS_LengthDimension.hxx>

using namespace MxFunctions;
namespace MechSectionShapeData
{

    class MXSECTIONSHAPEDATA_EXPORT mSectionShapeBase
    {
    public:
        /*
        * 重载构造函数
        */
        mSectionShapeBase(SecType type,const QString& name="", int id=0);

        /*
        * 析构函数
        */
        virtual ~mSectionShapeBase();
      
        virtual mSectionShapeBase* clone() const;
        /*
       * 设置ID
       */
        void setSectionShapeId(int id);
        /*
        * 获取ID
        */
        int  getSectionShapeId();

        /*
        * 设置类型
        */
        void setSectionShapeType(SecType type);
        /*
        * 获取类型
        */
        SecType  getSectionShapeType() const;
       
        /*
        * 设置截面名称
        */
        void setSectionShapeName(const QString& name);
        /*
        * 获取截面名称
        */
        QString getSectionShapeName();

        QString getSectionShapeTypeStr();
        
        
        /*
        * 拷贝分析属性
        */
        mSectionShapeBase* copySectionShape(const int& id, const QString& name);

        /*
        * 获取截面属性
        */
        const QList<double>& getData();
        /*
        * 清空截面属性
        */
        void clearData();
        /*
        * 获取截面参数
        */
        const QList<double> getPara();
        /*
        * 获取截面参数不含类型
        */
        const QList<double> getParaNoType();
        /*
        * 设置截面参数
        */
        void setPara(QList<double>& para);
        //暂时将mSectionShapeBase的接口放在这
        //void BaseshowBeamSec(Handle(AIS_Shape) aisshape); //OA dormant display
        void getDataFromBeam(QString name, void* beamShape); //OA

        void setNsm(QString nsm);

        QString getNsm();

        /*
        * 计算参数,创建截面形状
        */
        virtual void CreateSec() {};

        friend QDataStream& operator>>(QDataStream& in, mSectionShapeBase& obj);
        friend QDataStream& InStreammSectionShapeBaseV3(QDataStream& in, mSectionShapeBase& obj);
        friend QDataStream& operator<<(QDataStream& out, mSectionShapeBase& obj);
        
    protected:

        QList<double> _para;//参数
        //QList<double> _paraNoType;//参数
        QList<double> _data;//属性

        double area, Iz, Iy, J, Iyz, K1, K2;//Z形用了这个
    //private:
        static int _sectionShapeId;
        //ID
        int _id;

        //名字
        QString _sectionShapeName;

        //形状类型
        MxFunctions::SecType _secType;

        //非结构参数
        QString _nsm;

    };
} // namespace MechSectionShapeData