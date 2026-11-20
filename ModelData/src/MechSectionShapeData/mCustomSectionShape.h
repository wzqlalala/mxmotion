#pragma once
#include "MXSectionShapeData_global.h"
#include <QString>
#include <vector>
#include "mPhysicalFieldEnum.h"
#include "mSectionShapeBase.h"

using namespace MxFunctions;
namespace MechSectionShapeData
{

    class MXSECTIONSHAPEDATA_EXPORT mCustomSectionShape : public MechSectionShapeData::mSectionShapeBase
    {
    public:
        /*
        * 重载构造函数
        */
        mCustomSectionShape(const QString& name="", int id=0);

        /*
        * 析构函数
        */
        virtual ~mCustomSectionShape();
      
        mCustomSectionShape* clone() const override;       
        
        /*
        * 拷贝分析属性
        */
        mSectionShapeBase* copySectionShape(const int& id, const QString& name);

        /*
        * 获取截面参数
        */
        const QList<QString> getCustomPara();

        /*
        * 设置截面参数
        */
        void setCustomPara(QList<QString>& para);

        /*
        * 计算参数,创建截面形状
        */
        void CreateSec() override;

        friend QDataStream& operator>>(QDataStream& in, mSectionShapeBase& obj);
        friend QDataStream& InStreammCustomSectionShapeV3(QDataStream& in, mCustomSectionShape& obj);
        friend QDataStream& operator<<(QDataStream& out, mSectionShapeBase& obj);
        
    protected:

        QList<QString> _customPara;//参数

    };
} // namespace MechSectionShapeData