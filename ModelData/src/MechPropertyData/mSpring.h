#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXPropertyData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QPair>
#include <QList>

#include "mPropertyBase.h"

namespace MechPropertyData
{
	class MXPROPERTYDATA_EXPORT mSpring : public mPropertyBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mSpring(int id,MxFunctions::PropertyType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mSpring() {}

		/*
		* 析构函数
		*/
		~mSpring();

		/*
		* 拷贝分析工况
		*/
		mSpring* copyProperty(const int& id, const QString& name) override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mSpring& mgr);
		friend QDataStream& InStreammSpringV3(QDataStream& in, mSpring& obj);
		friend QDataStream& operator<<(QDataStream& out, mSpring& mgr);

	public:

		//刚度分量 (x/y/z/rx/ry/rz)
		QString _k;

		//属性 (线性/非线性)
		QString _nonlinear;

		//常刚度
		QString _value;

		//力-位移曲线
		QList<QPair<double, double>> _luCurve;

		//弹性属性 (单自由度弹簧)
		QString _K;

		//阻尼系数 (单自由度弹簧)
		QString _GE;

		//应力系数 (单自由度弹簧)
		QString _S;

		//弹簧类型 (单自由度弹簧/三自由度两弹簧/六自由度接地弹簧/六自由度两节点弹簧)
		QString _springCategory;
	};
}



