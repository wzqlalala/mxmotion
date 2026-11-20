#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXContactData_global.h"
#include <map>
#include <vector>
#include <QStringList>

#include "mPhysicalFieldEnum.h"
namespace MechContactData
{
	class mContactBase;
	class MXCONTACTDATA_EXPORT mContactManager
	{
	public:
		mContactManager();

		~mContactManager();
	
		/*
		* 通过id获取接触数据
		*/
		mContactBase* getContactData(int id);

		/*
		* 通过名字获取接触数据
		*/
		mContactBase* getContactData(const QString& name);

		/*
		* 通过id获取名称
		*/
		QString getContactNameByID(int id);

		/*
		* 通过名称获取id
		*/
		int getContactIdByName(QString name);

		/*
		* 添加接触数据
		*/
		void AddContactData(mContactBase* base);

		/*
		* 判断接触名是否存在
		*/
		bool ExistContactName(const QString& name);

		/*
		* 重命名接触
		*/
		void resetName(const QString& oldName, const QString& newName);

		/*
		* 拷贝接触
		*/
		int copyContactData(const int& id);

		/*
		* 删除所有接触
		*/
		void deleteAllContact();

		/*
		* 删除接触
		*/
		void deleteContact(const int& id);

		/*
		* 获取所有接触ID
		*/
		std::vector<int> getAllContactID();

		/*
		* 获取所有接触名称
		*/
		QStringList getAllContactNames();

		/*
		* 获取所有接触名称根据类型
		*/
		QStringList getAllContactNamesByTypes(MxFunctions::ContactType type);

		/*
		* 获取所有接触id根据类型
		*/
		std::vector<int> getAllContactIdsByTypes(MxFunctions::ContactType type);

		/*
		* 返回创建的新ID
		*/
		int createID();

		friend QDataStream& operator>>(QDataStream& in, mContactManager& obj);
		friend QDataStream& InStreammContactManagerV3(QDataStream& in, mContactManager& mgr);
		friend QDataStream& operator<<(QDataStream& out, mContactManager& obj);
	private:
		std::map<int, mContactBase*> _propertyData;

		//信号

		int _maxID;
	};
}