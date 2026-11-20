#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXDiscreteFieldData_global.h"
#include <map>
#include <QStringList>
#include <vector>
#include "mPhysicalFieldEnum.h"

namespace MechDiscreteFieldData
{
	class mFieldBase;
	class mDiscreteField;
	class mAnalysisField;

	/*
	* 场数据统一管理类：离散场（FieldType::Discrete）与分析场（FieldType::Analysis）
	* 统一存储、统一ID空间，按子类型过滤提供各自的访问接口。
	*/
	class MXDISCRETEFIELDDATA_EXPORT mFieldManager
	{
	public:
		mFieldManager();

		~mFieldManager();

		/*
		* 通过id获取场数据（基类）
		*/
		mFieldBase* getFieldData(int id);

		/*
		* 通过id获取离散场数据（类型不符返回nullptr）
		*/
		mDiscreteField* getDiscreteFieldData(int id);

		/*
		* 通过名字获取离散场数据
		*/
		mDiscreteField* getDiscreteFieldData(const QString& name);

		/*
		* 通过id获取分析场数据（类型不符返回nullptr）
		*/
		mAnalysisField* getAnalysisFieldData(int id);

		/*
		* 通过名字获取分析场数据
		*/
		mAnalysisField* getAnalysisFieldData(const QString& name);

		/*
		* 通过id获取名称
		*/
		QString getFieldNameByID(int id);

		/*
		* 通过名称获取id（匹配指定子类型；不限子类型时传 nullptr）
		*/
		int getIDByFieldName(const QString& name, const MxFunctions::FieldType* fieldType = nullptr);

		/*
		* 添加场数据
		*/
		void addFieldData(mFieldBase* base);

		/*
		* 判断场名是否存在（限定子类型）
		*/
		bool existFieldName(const QString& name, MxFunctions::FieldType fieldType);

		/*
		* 重命名场（限定子类型）
		*/
		void resetName(const QString& oldName, const QString& newName, MxFunctions::FieldType fieldType);

		/*
		* 拷贝场
		*/
		int copyFieldData(const int& id);

		/*
		* 删除指定子类型的所有场
		*/
		void deleteAllField(MxFunctions::FieldType fieldType);

		/*
		* 删除场
		*/
		void deleteField(const int& id);

		/*
		* 获取指定子类型的所有场ID
		*/
		std::vector<int> getAllFieldID(MxFunctions::FieldType fieldType);

		/*
		* 获取指定子类型的所有场名称
		*/
		QStringList getAllFieldNames(MxFunctions::FieldType fieldType);

		/*
		* 返回创建的新ID
		*/
		int createID();

		friend QDataStream& operator>>(QDataStream& in, mFieldManager& mgr);
		friend QDataStream& InStreammFieldManagerV3(QDataStream& in, mFieldManager& mgr);
		friend QDataStream& operator<<(QDataStream& out, mFieldManager& mgr);

	private:
		//所有场数据（离散场与分析场统一存储）
		std::map<int, mFieldBase*> _fieldDataMap;

		int _maxID;
	};
}
