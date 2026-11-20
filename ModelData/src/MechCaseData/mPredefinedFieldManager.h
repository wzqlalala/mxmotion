#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include <map>
#include <QStringList>
#include <memory>
#include <set>
#include <vector>
#include "mPhysicalFieldEnum.h"

namespace MechCaseData
{
	class mPredefinedFieldBase;
	class MXCASEDATA_EXPORT mPredefinedFieldManager
	{
	public:
		mPredefinedFieldManager();

		~mPredefinedFieldManager();

		/*
		* 通过id获取预定义场数据
		*/
		std::shared_ptr<mPredefinedFieldBase> getPredefinedFieldData(int id);

		/*
		* 通过名称获取预定义场数据
		*/
		std::shared_ptr<mPredefinedFieldBase> getPredefinedFieldData(const QString& name);

		/*
		* 通过id获取名称
		*/
		QString getPredefinedFieldNameByID(int id);

		/*
		* 通过id获取预定义场类型名称
		*/
		QString getPredefinedFieldTypeNameByID(int id);

		/*
		* 通过名称获取id
		*/
		int getIDByPredefinedFieldName(QString name);

		/*
		* 添加预定义场数据
		*/
		void AddPredefinedFieldData(std::shared_ptr<mPredefinedFieldBase> base);

		/*
		* 判断预定义场名是否存在
		*/
		bool ExistPredefinedFieldName(const QString& name);

		/*
		* 重命名预定义场
		*/
		void resetName(const QString& oldName, const QString& newName);

		/*
		* 拷贝预定义场
		*/
		int copyPredefinedFieldData(const int& id);

		/*
		* 删除所有预定义场
		*/
		void deleteAllPredefinedField();

		/*
		* 根据id删除预定义场
		*/
		void deletePredefinedField(const int& id);

		/*
		* 获取所有预定义场id
		*/
		std::vector<int> getAllPredefinedFieldID();

		/*
		* 获取所有预定义场名称
		*/
		QStringList getAllPredefinedFieldNames();

		/*
		* 获取所有预定义场名称根据类型
		*/
		QStringList getAllPredefinedFieldNamesByType(MxFunctions::LoadBoundaryType type);

		/*
		* 返回创建的新ID
		*/
		int createID();

		/*
		* 区分预定义场类型 
		*/
		void differentPredefinedFieldFlag(const std::set<int>& pds, std::vector<int>& ic, std::vector<int>& temp);

		friend QDataStream& operator>>(QDataStream& in, mPredefinedFieldManager& obj);
		friend QDataStream& InStreammPredefinedFieldManagerV3(QDataStream& in, mPredefinedFieldManager& mgr);
		friend QDataStream& operator<<(QDataStream& out, mPredefinedFieldManager& obj);
	private:
		std::map<int, std::shared_ptr<mPredefinedFieldBase>> _predefinedFieldData;
		int _maxID;
	};
}
