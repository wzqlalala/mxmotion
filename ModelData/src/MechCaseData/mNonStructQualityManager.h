#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include <map>
#include <QStringList>
#include <vector>
#include "mPhysicalFieldEnum.h"
namespace MechCaseData
{
	class mNonStructuralQuality;
	class MXCASEDATA_EXPORT mNonStructQualityManager
	{
	public:
		mNonStructQualityManager();

		~mNonStructQualityManager();
	
		/*
		* 通过id获取非结构质量数据
		*/
		mNonStructuralQuality* getNonStructuralQualityData(int id);

		/*
		* 通过名字非结构质量数据
		*/
		mNonStructuralQuality* getNonStructuralQualityData(const QString& name);

		/*
		* 通过id获取名称
		*/
		QString getNonStructuralQualityNameByID(int id);

		/*
		* 通过名称获取id
		*/
		int getIDByNsmName(QString name);
		

		/*
		* 添加分析非结构质量数据
		*/
		void AddNsmData(mNonStructuralQuality* base);

		/*
		* 判断分析非结构质量名是否存在
		*/
		bool ExistNsmName(const QString& name);

		/*
		* 重命名非结构质量
		*/
		void resetName(const QString& oldName, const QString& newName);

		/*
		* 拷贝非结构质量
		*/
		int copyNsmData(const int& id);

		/*
		* 删除所有非结构质量
		*/
		void deleteAllNsm();

		/*
		* 删除非结构质量
		*/
		void deleteNsm(const int& id);

		/*
		* 获取所有非结构质量ID
		*/
		std::vector<int> getAllNonSturcturalQualityID();

		/*
		* 获取所有非结构质量名称
		*/
		QStringList getAllNsmNames();

		/*
		* 获取所有非结构质量名称根据类型
		*/
		//QStringList getAllPropertyNamesByTypes(MxFunctions::PropertyType type);
		/*
		* 返回创建的新ID
		*/
		int createID();

		friend QDataStream& operator>>(QDataStream& in, mNonStructQualityManager& mgr);
		friend QDataStream& InStreammNonStructQualityManagerV3(QDataStream& in, mNonStructQualityManager& mgr);
		friend QDataStream& operator<<(QDataStream& out, mNonStructQualityManager& mgr);
	private:
		std::map<int, mNonStructuralQuality*> _nsmDataMap;

		//信号

		int _maxID;
	};
}