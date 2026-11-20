#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXModelFixData_global.h"
#include <map>
#include <QStringList>
#include <memory>

#include "mPhysicalFieldEnum.h"
#include "mModelFixDataBase.h"

namespace MechModelFixData
{
	class mModelFixDataBase;
	class MXMODELFIXDATA_EXPORT mModelFixManager
	{
	public:
		mModelFixManager();

		~mModelFixManager();

		/*
		* 通过id获取模型修复数据
		*/
		std::shared_ptr<mModelFixDataBase> getModelFixData(int id);

		/*
		* 通过名字获取模型修复数据
		*/
		std::shared_ptr<mModelFixDataBase> getModelFixData(const QString& name);

		/*
		* 通过id获取名称
		*/
		QString getModelFixNameByID(int id);

		/*
		* 通过名称获取id
		*/
		int getIDByModelFixName(QString name);

		/*
		* 添加模型修复数据
		*/
		void AddModelFixData(std::shared_ptr<mModelFixDataBase> base);

		/*
		* 判断模型修复名是否存在
		*/
		bool ExistModelFixName(const QString& name);

		/*
		* 重命名模型修复
		*/
		void resetName(const QString& oldName, const QString& newName);

		/*
		* 拷贝模型修复
		*/
		int copyModelFixData(const int& id);

		/*
		* 删除所有模型修复
		*/
		void deleteAllModelFix();

		/*
		* 删除模型修复
		*/
		void deleteModelFix(const int& id);

		/*
		* 获取所有模型修复ID
		*/
		std::vector<int> getAllModelFixID();

		/*
		* 获取所有模型修复名称
		*/
		QStringList getAllModelFixNames();

		/*
		* 返回创建的新ID
		*/
		int createID();

		friend QDataStream& operator>>(QDataStream& in, mModelFixManager& mgr);
		friend QDataStream& InStreammModelFixManagerV3(QDataStream& in, mModelFixManager& mgr);
		friend QDataStream& operator<<(QDataStream& out, mModelFixManager& mgr);

	private:
		std::map<int, std::shared_ptr<mModelFixDataBase>> _modelFixDataMap;

		int _maxID;
	};
}
