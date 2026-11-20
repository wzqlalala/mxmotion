#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include <map>
#include <QStringList>
#include <memory>
#include <set>
#include "mUnitSystemManager.h"
#include "mPhysicalFieldEnum.h"

using namespace CommonData;
namespace MechCaseData
{
	class mFreqSamplPointBase;
	class MXCASEDATA_EXPORT mFreqSamplPointManager
	{
	public:
		mFreqSamplPointManager();

		~mFreqSamplPointManager();

		/*
		* 通过id获取频率点采样数据
		*/
		mFreqSamplPointBase* getFreqSamplPointData(int id);

		/*
		* 通过名称获取载荷边界数据
		*/
		mFreqSamplPointBase* getFreqSamplPointData(const QString& name);

		/*
		* 通过id获取名称
		*/
		QString getFreqSamplPointNameByID(int id);


		/*
		* 通过名称获取id
		*/
		int getIDByFreqSamplPointName(QString name);

		/*
		* 添加数据
		*/
		void AddFreqSamplPointData(mFreqSamplPointBase* base);


		/*
		* 判断名是否存在
		*/
		bool ExistFreqSamplPointName(const QString& name);

		/*
		* 重命名
		*/
		void resetName(const QString& oldName, const QString& newName);

		/*
		* 拷贝
		*/
		int copyFreqSamplPointData(const int& id);

		/*
		* 删除所有
		*/
		void deleteAllFreqSamplPoint();


		/*
		* 根据id删除
		*/
		void deleteFreqSamplPoint(const int& id);

		/*
		* 获取所有ID
		*/
		std::vector<int> getAllFreqSamplPointID();

		/*
		* 获取所有名称
		*/
		QStringList getAllFreqSamplPointNames();

		/*
		* 返回创建的新ID
		*/
		int createID();

		friend QDataStream& operator>>(QDataStream& in, mFreqSamplPointManager& obj);
		friend QDataStream& InStreammFreqSamplPointManagerV3(QDataStream& in, mFreqSamplPointManager& mgr);
		friend QDataStream& operator<<(QDataStream& out, mFreqSamplPointManager& obj);
	private:
		std::map<int, mFreqSamplPointBase*> _freqSamplPointData;
		int _maxID;
	};
}