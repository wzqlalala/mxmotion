#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXPropertyData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mModelEnum.h"
#include <QString>
#include <QPair>
#include <QVector>

#include "mNodeValidor.h"

namespace MechPropertyData
{
	class MXPROPERTYDATA_EXPORT mPropertyBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mPropertyBase(int id,MxFunctions::PropertyType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mPropertyBase() {}

		/*
		* 析构函数
		*/
		virtual ~mPropertyBase();

		/*
		* 设置属性ID
		*/
		void setPropertyId(int id);

		/*
		* 获取属性ID
		*/
		int getPropertyId();

		/*
		* 设置属性名
		*/
		void setPropertyName(const QString& name);

		/*
		* 获取属性名
		*/
		QString getPropertyName();

		/*
		* 设置属性类型
		*/
		void setPropertyType(MxFunctions::PropertyType type);
		/*
		* 获取属性类型
		*/
		MxFunctions::PropertyType getPropertyType();

		/*
		* 获取属性英文
		*/
		QString propertyTypeToStringEn();

		/*
		* 获取属性类型的字符串
		*/
		QString getPropertyTypeStr();

		/*
		 * 设置属性的描述
		 */
		void setPropertyDescribe(const QString& describe);

		/*
		 * 获取属性的描述
		 */
		QString getPropertyDescribe();

		/*
		* 拷贝分析属性
		*/
		virtual mPropertyBase* copyProperty(const int & id,const QString& name)=0;

		/*
		* 设置方向角度
		*/
		virtual void setAngle(QString angle)  {};

		/*
		* 获取方向角度
		*/
		virtual QString getAngle() { return ""; };

		/*
		* 设置厚度
		*/
		virtual void setThickness(QString thickness) {};

		/*
		* 获取厚度
		*/
		virtual QString getThickness() { return ""; };

		/*
		* 判断数据完整性
		*/
		virtual MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) { 
			return MxFunctions::BrowserTreeStatus::Tree_Yes;
		};

		friend QDataStream& operator>>(QDataStream& in, mPropertyBase& mgr);
		friend QDataStream& InStreammPropertyBaseV3(QDataStream& in, mPropertyBase& obj);
		friend QDataStream& operator<<(QDataStream& out, mPropertyBase& mgr);

	private:
		
		//属性ID
		int _id;

		//属性名字
		QString _propertyName;

		//属性类型
		MxFunctions::PropertyType _propertyType;

		//属性描述
		QString _propertyDescribe;
	};
}
