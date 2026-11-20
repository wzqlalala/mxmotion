#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXPropertyData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QVector>
#include <QMap>

#include "mPropertyBase.h"

namespace MechPropertyData
{
	class MXPROPERTYDATA_EXPORT mPlyLaminate : public mPropertyBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mPlyLaminate(int id, MxFunctions::PropertyType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mPlyLaminate() {}

		/*
		* 析构函数
		*/
		~mPlyLaminate();

		/*
		* 拷贝分析工况
		*/
		mPlyLaminate* copyProperty(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mPlyLaminate& mgr);
		friend QDataStream& InStreammPlyLaminateV3(QDataStream& in, mPlyLaminate& obj);
		friend QDataStream& operator<<(QDataStream& out, mPlyLaminate& mgr);

	public:
		/**************************************共享变量***********************************************/
		int _type;       //0壳 1实体壳 2实体
		int _plyDefType; //0直接定义 1层合板
		int _materialDir;//0全局坐标系，1局部坐标系，2离散坐标系(暂无)
		/*法向 （默认 3）
		*  x  y  z
		*  1  2  3
		*/
		int _normaldirection;

		/*
		* 转角(固定角度)
		*/
		QString _rotAngle;

		/*
		* 层合结构定义表格->厚度
		*/
		QVector<QString>  thicknessList;

		/*
		* 层合结构定义表格->角度
		*/
		QVector<QString>  angleList;

		/*
		* 层合结构定义表格->积分点数
		*/
		QVector<QString>  intePointList;

		/**************************************壳变量***********************************************/

		/*1.壳属性（默认 无）
		* 无、对称铺层、弯曲行为、膜行为、等效单层、对称弯曲、对称膜、对称等效单层
		*	  sym、     bend、    mem、   smear、   sybend、  symem、 sysmear
		*/
		QString _shellProperty;

		/*2.参考面位置 （默认 无）
		* 无、偏置距离、偏置率、顶部、底部
		*     zoffs、    ratio、    top、 bottom
		*/
		QString _referenceSurfacePos;

		//3.参考面偏置距离/偏置率(参考面位置选择偏置距离或偏置率时有 默认 0)
		QString _referenceSurfaceDis;

		/*4.失效（默认 无）
		* 无、最大应力、 最大应变、  Tsai-Hill、Tsai-Wu、Azzi-Tsai-Hill、Hoff、Puck、Hashin
		*    max_stress、max_strain、tsai_hill、tsai_wu、azzi_tasi_hill、hoff、puck、hfail
		*/
		QString _failure;

		/*4.厚度（默认 截面厚度）
		* 截面厚度、单元厚度离散场、节点厚度离散场
		*/
		QString _thickness;

		/**************************************实体壳变量***********************************************/
		/*实体壳属性（默认 无）
		* 无、对称
		*     sym
		*/
		QString _solidShellProperty;

		/**************************************实体变量***********************************************/
		/*实体属性（默认无）
		* 无、对称
		*     sym
		*/
		QString _solidProperty;

		/*实体堆叠方向(默认 3)
		* 单元方向1、单元方向2、单元方向3，默认单元方向3
		* 1          2          3
		*/
		int eleMentDir;

		/**************************************材料方向坐标系***********************************************/
		int _rotAngleType; //0无 1固定角度 //2离散场
	};
}