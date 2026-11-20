#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXFunctionData_global.h"
#include <map>
#include <vector>
#include <QString>
#include <QStringList>

#include "mFunctionBase.h"


namespace MechFunctionData
{
	class mFunctionBase;
	class MXFUNCTIONDATA_EXPORT mFunctionManager
	{
	public:
		mFunctionManager();


		~mFunctionManager();

		/*
		* 通过id获取坐标系数据
		*/
		mFunctionBase* getFunctionData(int id);

		/*
		* 通过id获取名称
		*/
		QString getFunctionNameByID(int id);

		/*
		* 通过名称获取id
		*/
		int getIDByFunctionName(QString name);

		/*
		* 添加函数数据
		*/
		void AddFunctionData(mFunctionBase* func);

		/*
		* 判断函数名是否存在
		*/
		bool ExistFunctionName(const QString& name);

		/*
		* 重命名函数
		*/
		void resetName(const QString& oldName, const QString& newName);

		/*
		* 返回创建的新ID
		*/
		int createID();

		/*
		* 获取所有函数名称
		*/
		QStringList getAllFunctionNames();

		/*
		* 获取所有函数ID
		*/
		std::vector<int> getAllFunctionID();

		/*
		* 删除所有函数
		*/
		void deleteAllFunction();

		/*
		* 删除函数
		*/
		void deleteFunction(const int& id);

		/*
		* 拷贝函数
		*/
		int copyFunctionData(const int& id);

		/*
		* 当前模型树Id
		*/
		int getMaxID();

		friend QDataStream& operator>>(QDataStream& in, mFunctionManager& mgr);
		friend QDataStream& InStreammFunctionManagerV3(QDataStream& in, mFunctionManager& mgr);
		friend QDataStream& operator<<(QDataStream& out, mFunctionManager& mgr);

	private:
		std::map<int, mFunctionBase*> _functionData;

		int _maxID;
	};



}