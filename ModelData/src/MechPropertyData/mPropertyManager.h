#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXPropertyData_global.h"
#include <map>
#include <QStringList>
#include <vector>
#include "mPhysicalFieldEnum.h"
namespace MechPropertyData
{
	class mPropertyBase;
	class MXPROPERTYDATA_EXPORT mPropertyManager
	{
	public:
		mPropertyManager();

		~mPropertyManager();
	
		/*
		* 通过id获取属性数据
		*/
		mPropertyBase* getPropertyData(int id);

		/*
		* 通过名字获取属性数据
		*/
		mPropertyBase* getPropertyData(const QString& name);

		/*
		* 通过id获取名称
		*/
		QString getPropertyNameByID(int id);

		/*
		* 通过名称获取id
		*/
		int getIDByPropertyName(QString name);
		

		/*
		* 添加分析属性数据
		*/
		void AddPropertyData(mPropertyBase* base);

		/*
		* 判断分析属性名是否存在
		*/
		bool ExistPropertyName(const QString& name);

		/*
		* 重命名属性
		*/
		void resetName(const QString& oldName, const QString& newName);

		/*
		* 拷贝属性
		*/
		int copyPropertyData(const int& id);

		/*
		* 删除所有属性
		*/
		void deleteAllProperty();

		/*
		* 删除属性
		*/
		void deleteProperty(const int& id);

		/*
		* 获取所有属性ID
		*/
		std::vector<int> getAllProperyID();

		/*
		* 获取所有属性名称
		*/
		QStringList getAllPropertyNames();

		/*
		* 获取所有属性名称根据类型
		*/
		QStringList getAllPropertyNamesByTypes(MxFunctions::PropertyType type);
		/*
		* 返回创建的新ID
		*/
		int createID();

		friend QDataStream& operator>>(QDataStream& in, mPropertyManager& mgr);
		friend QDataStream& InStreammPropertyManagerV3(QDataStream& in, mPropertyManager& mgr);
		friend QDataStream& operator<<(QDataStream& out, mPropertyManager& mgr);
	private:
		std::map<int, mPropertyBase*> _propertyData;

		//信号

		int _maxID;
	};
}