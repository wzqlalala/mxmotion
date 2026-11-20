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
	class mPeriodicBoundary;
	class MXCONTACTDATA_EXPORT mPeriodicBoundaryManager
	{
	public:
		mPeriodicBoundaryManager();
		~mPeriodicBoundaryManager();

		/*
		* 通过id获取数据
		*/
		mPeriodicBoundary* getPeriodicBoundaryData(int id);

		/*
		* 通过名字获取数据
		*/
		mPeriodicBoundary* getPeriodicBoundaryData(const QString& name);

		/*
		* 通过id获取名称
		*/
		QString getPeriodicBoundaryNameByID(int id);

		/*
		* 通过名称获取id
		*/
		int getPeriodicBoundaryIdByName(const QString& name);

		/*
		* 添加数据
		*/
		void AddPeriodicBoundaryData(mPeriodicBoundary* base);

		/*
		* 判断名称是否存在
		*/
		bool ExistPeriodicBoundaryName(const QString& name);

		/*
		* 重命名
		*/
		void resetName(const QString& oldName, const QString& newName);

		/*
		* 拷贝
		*/
		int copyPeriodicBoundaryData(const int& id);

		/*
		* 删除所有
		*/
		void deleteAllPeriodicBoundary();

		/*
		* 删除指定
		*/
		void deletePeriodicBoundary(const int& id);

		/*
		* 获取所有ID
		*/
		std::vector<int> getAllPeriodicBoundaryID();

		/*
		* 获取所有名称
		*/
		QStringList getAllPeriodicBoundaryNames();

		/*
		* 获取所有名称根据类型
		*/
		QStringList getAllPeriodicBoundaryNamesByTypes(MxFunctions::PeriodicBoundaryType type);

		/*
		* 获取所有id根据类型
		*/
		std::vector<int> getAllPeriodicBoundaryIdsByTypes(MxFunctions::PeriodicBoundaryType type);

		/*
		* 创建新ID
		*/
		int createID();

		friend QDataStream& operator>>(QDataStream& in, mPeriodicBoundaryManager& obj);
		friend QDataStream& InStreammPeriodicBoundaryManagerV3(QDataStream& in, mPeriodicBoundaryManager& mgr);
		friend QDataStream& operator<<(QDataStream& out, mPeriodicBoundaryManager& obj);

	private:
		std::map<int, mPeriodicBoundary*> _propertyData;
		int _maxID;
	};
}
