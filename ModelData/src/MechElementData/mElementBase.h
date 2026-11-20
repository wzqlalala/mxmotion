#pragma once

#include "MXElementData_global.h"

#include "mElementFunction.h"
#include "mOtherFunction.h"


//Qt
#include <QVector3D>
#include <QString>
#include <QStringList>

using namespace MxFunctions;
namespace MechElementData
{

	//单元数据基类
	class MXELEMENTDATA_EXPORT mElementBase
	{
	public:
		mElementBase();

		mElementBase(ElementType et);

		ElementType getElementType() { return elementType; };

		void setElementType(ElementType et) {  elementType=et; };

		virtual QString getParameter() { return ""; };

		friend QDataStream& operator>>(QDataStream& in, mElementBase& mgr);
		friend QDataStream& InStreammElementBaseV3(QDataStream& in, mElementBase& obj);
		friend QDataStream& operator<<(QDataStream& out, mElementBase& mgr);

	public:
		ElementType elementType = MxFunctions::NoneElement;
	};

	//质量单元Mass1
	class MXELEMENTDATA_EXPORT mMassElement : public mElementBase
	{
	public:

		mMassElement():mElementBase(Mass1) { };

		mMassElement(double mass) :mElementBase(Mass1) { _mass = mass; };

		~mMassElement() {};

		QString getParameter() override
		{
			if (!_x.isEmpty() && !_y.isEmpty()&& !_z.isEmpty()&& !_I11.isEmpty()&& !_I21.isEmpty()&&
				!_I22.isEmpty()&& !_I31.isEmpty()&& !_I32.isEmpty()&& !_I33.isEmpty())
			{
				return QString("%1, %2, %3, %4, %5, %6, %7, %8, %9, %10").arg(_mass).arg(_x).arg(_y).arg(_z)
					.arg(_I11).arg(_I21).arg(_I22).arg(_I31).arg(_I32).arg(_I33);
			}
			else
			{
				return _mass;
			}
		};

		friend QDataStream& operator>>(QDataStream& in, mMassElement& mgr);
		friend QDataStream& InStreammMassElementV3(QDataStream& in, mMassElement& obj);
		friend QDataStream& operator<<(QDataStream& out, mMassElement& mgr);

		QString _mass = "0.01";

		//须同时给定与不给
		QString _x;	
		QString _y;
		QString _z;
		QString _I11;
		QString _I21;
		QString _I22;
		QString _I31;
		QString _I32;
		QString _I33;
	};

	//质量单元Cmass2
	class MXELEMENTDATA_EXPORT mCmass2Element : public mElementBase
	{
	public:

		mCmass2Element() :mElementBase(CMASS2) { };

		~mCmass2Element() {};

		QString getParameter() override
		{
			return QString("%1, %2").arg(_mass).arg(_dof);
		};

		friend QDataStream& operator>>(QDataStream& in, mCmass2Element& mgr);
		friend QDataStream& InStreammCmass2ElementV3(QDataStream& in, mCmass2Element& obj);
		friend QDataStream& operator<<(QDataStream& out, mCmass2Element& mgr);

		//类型
		QString _mass = "0.01";
		QString _dof = "1";
	};

	//隐式梁单元
	class MXELEMENTDATA_EXPORT mBeamElement : public mElementBase
	{
	public:

		mBeamElement() :mElementBase(MxFunctions::D3B2) { };

		mBeamElement(ElementType et) :mElementBase(et) { };

		~mBeamElement() {};

		QString getParameter() override
		{
			return "";
		};

		friend QDataStream& operator>>(QDataStream& in, mBeamElement& mgr);
		friend QDataStream& InStreammBeamElementV3(QDataStream& in, mBeamElement& obj);
		friend QDataStream& operator<<(QDataStream& out, mBeamElement& mgr);

		//矢量
		QString _beamValue;
		//节点
		QString _node;

		//偏置类型
		QString _biasType;
		QString _X1;
		QString _Y1;
		QString _Z1;
		QString _X2;
		QString _Y2;
		QString _Z2;
		QString _pinsa;
		QString _pinsb;
	};

	//隐式壳单元
	class MXELEMENTDATA_EXPORT mShellElement : public mElementBase
	{
	public:

		mShellElement() :mElementBase(TRIA3) {  };

		mShellElement(ElementType et) :mElementBase(et) {  };

		~mShellElement() {};

		QString getParameter() override
		{
			return QString();
		};

		friend QDataStream& operator>>(QDataStream& in, mShellElement& mgr);
		friend QDataStream& InStreammShellElementV3(QDataStream& in, mShellElement& obj);
		friend QDataStream& operator<<(QDataStream& out, mShellElement& mgr);

		//可选参数，""，"材料坐标系"，"材料转角值"
		QString _paraType = "";

		//坐标系
		QString _coorName = ""; //临时数据（读文件使用）
		int _coordID = 0;

		//角度
		QString _angle;

		QString _bias;
	};

	//弹簧阻尼单元
	class MXELEMENTDATA_EXPORT mElas1Element : public mElementBase
	{
	public:

		mElas1Element() :mElementBase(ElementType::ELAS1) {  };

		mElas1Element(ElementType et) :mElementBase(et) {  };

		~mElas1Element() {};

		QString getParameter() override
		{
			return QString("%1").arg(_dof, 10);
		};

		friend QDataStream& operator>>(QDataStream& in, mElas1Element& mgr);
		friend QDataStream& InStreammElas1ElementV3(QDataStream& in, mElas1Element& obj);
		friend QDataStream& operator<<(QDataStream& out, mElas1Element& mgr);

		QString _dof = "1";
	};

	//cbush单元
	class MXELEMENTDATA_EXPORT mCbushElement : public mElementBase
	{
	public:

		mCbushElement() :mElementBase(ElementType::CBUSH) {  };

		mCbushElement(ElementType et) :mElementBase(et) {  };

		~mCbushElement() {};

		QString getParameter() override
		{
			return QString();
		};

		friend QDataStream& operator>>(QDataStream& in, mCbushElement& mgr);
		friend QDataStream& InStreammCbushElementV3(QDataStream& in, mCbushElement& obj);
		friend QDataStream& operator<<(QDataStream& out, mCbushElement& mgr);

		//坐标系
		QString _cidName = ""; //临时数据（读文件使用）
		int _cid = 0;

		//偏置值（默认值为0.5，0至1之间的实数）
		QString _s;

		//偏置向量坐标系ID
		QString _ocidName = ""; //临时数据（读文件使用）
		int _ocid = 0;

		//方向向量
		QString _x1;
		QString _x2;
		QString _x3;

		//在OCID坐标系中的分量偏移量
		QString _si;
	};

}