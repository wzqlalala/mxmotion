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
	class mRoatateAnalyseBase;
	class MXCASEDATA_EXPORT mRotateAnalyseManager
	{
	public:
		mRotateAnalyseManager();

		~mRotateAnalyseManager();

		/*
		* 通过id获取旋转分析数据
		*/
		std::shared_ptr<mRoatateAnalyseBase> getRotateAnalyseData(int id);

		/*
		* 通过名称获取旋转分析数据
		*/
		std::shared_ptr<mRoatateAnalyseBase> getRotateAnalyseData(const QString& name);

		/*
		* 通过id获取名称
		*/
		QString getRotateAnalyseNameByID(int id);

		/*
		* 通过id获取旋转分析类型名称
		*/
		QString getRotateAnalyseTypeNameByID(int id);

		/*
		* 通过名称获取id
		*/
		int getIDByRotateAnalyseName(QString name);

		/*
		* 添加旋转分析数据
		*/
		void AddRotateAnalyseData(std::shared_ptr<mRoatateAnalyseBase> base);

		/*
		* 判断旋转分析名是否存在
		*/
		bool ExistRotateAnalyseName(const QString& name);

		/*
		* 重命名旋转分析
		*/
		void resetName(const QString& oldName, const QString& newName);

		/*
		* 拷贝旋转分析
		*/
		int copyRotateAnalyseData(const int& id);

		/*
		* 删除所有旋转分析
		*/
		void deleteAllRotateAnalyse();

		/*
		* 根据id删除旋转分析
		*/
		void deleteRotateAnalyse(const int& id);

		/*
		* 获取所有旋转分析id
		*/
		std::vector<int> getAllRotateAnalyseID();

		/*
		* 获取所有旋转分析名称
		*/
		QStringList getAllRotateAnalyseNames();

		/*
		* 获取所有旋转分析名称根据类型
		*/
		QStringList getAllRotateAnalyseNamesByType(MxFunctions::RotateAnalysyType type);

		/*
		* 返回创建的新ID
		*/
		int createID();

		friend QDataStream& operator>>(QDataStream& in, mRotateAnalyseManager& obj);
		friend QDataStream& InStreammRotateAnalyseManagerV3(QDataStream& in, mRotateAnalyseManager& mgr);
		friend QDataStream& operator<<(QDataStream& out, mRotateAnalyseManager& obj);
	private:
		std::map<int, std::shared_ptr<mRoatateAnalyseBase>> _rotateAnalyseData;
		int _maxID;
	};
}
