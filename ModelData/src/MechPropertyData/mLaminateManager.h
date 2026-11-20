#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXPropertyData_global.h"
#include <map>
#include <QStringList>
#include <memory>
#include <set>
#include <vector>
#include "mPhysicalFieldEnum.h"

#include "mLaminateBase.h"

namespace MechPropertyData
{
	class MXPROPERTYDATA_EXPORT mLaminateManager
	{

	public:

		mLaminateManager();

		~mLaminateManager();

		/*
		* 通过id获取数据
		*/
		mLaminateBase* getLaminateData(int id);

		/*
		* 通过名称获取数据
		*/
		mLaminateBase* getLaminateData(const QString& name);

		/*
		* 通过id获取名称
		*/
		QString getLaminateNameByID(int id);

		/*
		* 通过名称获取id
		*/
		int getIDByLaminateName(QString name);

		/*
		* 添加数据
		*/
		void AddLaminateData(mLaminateBase* base);

		/*
		* 判断名是否存在
		*/
		bool ExistLaminateName(const QString& name);

		/*
		* 重命名载荷边界
		*/
		void resetName(const QString& oldName, const QString& newName);

		/*
		* 拷贝载荷
		*/
		int copyLaminateData(const int& id);

		/*
		* 删除所有
		*/
		void deleteAllLaminate();

		/*
		* 根据id删除
		*/
		void deleteLaminateById(const int& id);

		/* 获取所有ID
		*/
		std::vector<int> getAllLaminateID();

		/*
		* 获取所有名称
		*/
		QStringList getAllLaminateName();

		/*
		* 获取所有名称根据类型
		*/
		QStringList getAllLaminateByTypes(MxFunctions::LaminateType type);

		/*
		* 返回创建的新ID
		*/
		int createID();

		friend QDataStream& operator>>(QDataStream& in, mLaminateManager& obj);
		friend QDataStream& InStreammLaminateManagerV3(QDataStream& in, mLaminateManager& mgr);
		friend QDataStream& operator<<(QDataStream& out, mLaminateManager& obj);

	private:
		std::map<int, mLaminateBase*> _LaminateData;
		int _maxID;
	};
}