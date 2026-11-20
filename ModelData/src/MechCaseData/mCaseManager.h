#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include <map>
#include <QStringList>
#include <memory>

#include "mGlobalSignals.h"
#include "mUnitSystemManager.h"
#include "mPhysicalFieldEnum.h"

using namespace CommonData;
namespace MechCaseData
{
	class mCaseBase;
	//class mLoadBase;
	//class mBoundaryBase;
	class MXCASEDATA_EXPORT mCaseManager
	{
	public:
		mCaseManager();

		~mCaseManager();
	
		/*
		* 通过id获取分析工况数据
		*/
		mCaseBase* getCaseData(int id);

		/*
		* 通过名称获取分析工况数据
		*/
		mCaseBase* getCaseData(const QString& name);

		/*
		* 通过id获取名称
		*/
		QString getCaseNameByID(int id);
		
		/*
		* 通过id获取类型名称
		*/
		QString getCaseTypeNameByID(int id);

		/*
		* 通过名称获取id
		*/
		int getIDByCaseName(QString name);

		/*
		* 添加分析工况数据
		*/
		void AddCaseData(mCaseBase* base);

		/*
		* 判断分析工况名是否存在
		*/
		bool ExistCaseName(const QString& name);

		/*
		* 重命名工况
		*/
		void resetName(const QString& oldName, const QString& newName);

		/*
		* 拷贝工况
		*/
		int copyCaseData(const int& id);

		/*
		* 删除所有工况
		*/
		void deleteAllCase();

		/*
		* 删除工况
		*/
		void deleteCase(const int& id);

		/*
		* 获取所有工况ID
		*/
		std::vector<int> getAllCaseID();

		/*
		* 获取所有工况名称
		*/
		QStringList getAllCaseNames();

		/*
		* 获取所有工况名称根据类型
		*/
		QStringList getAllCaseNamesByTypes(MxFunctions::CaseType type);

		/*
		* 返回创建的新ID
		*/
		int createID();

		friend QDataStream& operator>>(QDataStream& in, mCaseManager& obj);
		friend QDataStream& InStreammCaseManagerV3(QDataStream& in, mCaseManager& mgr);
		friend QDataStream& operator<<(QDataStream& out, mCaseManager& obj);
	
	private:
		std::map<int, mCaseBase*> _caseData;

		//信号
		static int _maxID;
	};
}