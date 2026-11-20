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
	class mLoadBoundaryBase;
	class MXCASEDATA_EXPORT mLoadBoundaryManager
	{
	public:
		mLoadBoundaryManager();

		~mLoadBoundaryManager();

		/*
		* 通过id获取载荷边界数据
		*/
		std::shared_ptr<mLoadBoundaryBase> getLoadBoundaryData(int id);

		/*
		* 通过名称获取载荷边界数据
		*/
		std::shared_ptr<mLoadBoundaryBase> getLoadBoundaryData(const QString& name);

		/*
		* 通过id获取名称
		*/
		QString getLoadBoundaryNameByID(int id);

		/*
		* 通过id获取载荷边界类型名称
		*/
		QString getLoadBoundaryTypeNameByID(int id);

		/*
		* 通过类型枚举获取中文类型
		*/
		QString getLoadBoundaryTypeNameByType(LoadBoundaryType type);

		/*
		* 通过名称获取id
		*/
		int getIDByLoadBoundaryName(QString name);

		/*
		* 添加载荷边界数据
		*/
		void AddLoadBoundaryData(std::shared_ptr<mLoadBoundaryBase> base);


		/*
		* 判断载荷边界名是否存在
		*/
		bool ExistLoadBoundaryName(const QString& name);

		/*
		* 重命名载荷边界
		*/
		void resetName(const QString& oldName, const QString& newName);

		/*
		* 拷贝载荷
		*/
		int copyLoadBoundaryData(const int& id);

		/*
		* 删除所有载荷
		*/
		void deleteAllLoad();

		/*
		* 删除所有边界
		*/
		void deleteAllBoundary();

		/*
		* 根据id删除载荷边界
		*/
		void deleteLoadBoundary(const int& id);

		/*
		* 获取所有载荷ID
		*/
		std::vector<int> getAllLoadID();

		/*
		* 获取所有边界ID
		*/
		std::vector<int> getAllBoundaryID();

		/*
		* 获取所有载荷边界id
		*/
		std::vector<int> getAllLoadBoundaryID();

		/*
		* 获取所有载荷名称
		*/
		QStringList getAllLoadNames();

		/*
		* 获取所有边界名称
		*/
		QStringList getAllBoundaryNames();

		/*
		* 获取所有载荷边界名称
		*/
		QStringList getAllLoadBoundaryName();

		/*
		* 获取所有载荷边界名称根据类型
		*/
		QStringList getAllLoadBoundaryNamesByTypes(MxFunctions::LoadBoundaryType type);

		/*
		* 返回创建的新ID
		*/
		int createID();

		/*
		* 响应单位制转换，载荷独有
		*/
		void responseUnitChanged(const int& unit, mUnitSystemManager* unitsystemmanager);

		/*
		* 区分载荷边界
		*/
		void differentiateLoadBoundary(const std::set<int>& loadBcIDs, std::vector<int>& load, std::vector<int>& boundary);

		/*
		* 区分载荷类型
		*/
		void differentiateLoadFlag(const std::set<int>& loadIDs, std::vector<int>& mechanicalLoad, std::vector<int>& dynamicLoad, bool onlyUnsuppressed = false);

		/*
		* 区分边界类型
		*/
		void differentiateBoundaryFlag(const std::set<int>& bcDs, std::vector<int>& spc, std::vector<int>& suport1, bool onlyUnsuppressed = false);

		/*
		* 判断是否存在超单元边界
		*/
		bool isExistGenericBoundary();


		friend QDataStream& operator>>(QDataStream& in, mLoadBoundaryManager& obj);
		friend QDataStream& InStreammLoadBoundaryManagerV3(QDataStream& in, mLoadBoundaryManager& mgr);
		friend QDataStream& operator<<(QDataStream& out, mLoadBoundaryManager& obj);
	private:
		std::map<int, std::shared_ptr<mLoadBoundaryBase>> _loadBoundaryData;
		int _maxID;
	};
}