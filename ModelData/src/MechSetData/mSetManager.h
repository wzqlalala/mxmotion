#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXSetData_global.h"
#include <map>
#include <QStringList>
#include <vector>
#include "mPhysicalFieldEnum.h"
namespace MechSetData
{
	class mSetBase;
	class MXSETDATA_EXPORT mSetManager
	{
	public:
		mSetManager();

		~mSetManager();
	
		/*
		* 通过id获取集合数据
		*/
		mSetBase* getSetData(int id);

		/*
		* 通过名称获取集合数据
		*/
		mSetBase* getSetData(const QString& name);

		/*
		* 通过id获取名称
		*/
		QString getSetNameByID(int id);

		/*
		* 通过名称获取id
		*/
		int getIDBySetName(QString name, bool isContainAutoCreate = false);

		/*
		* 添加集合数据
		*/
		void AddSetData(mSetBase* base);

		/*
		* 更新集合数据
		*/
		void replaceSetData(mSetBase* base);

		/*
		* 判断集合名是否存在
		*/
		bool ExistSetName(const QString& name, bool isInternal = false);

		/*
		* 重命名集合
		*/
		void resetName(const QString& oldName, const QString& newName);

		/*
		* 拷贝集合
		*/
		int copySetData(const int& id);

		/*
		* 删除所有集合
		*/
		void deleteAllSet();

		/*
		* 删除集合
		*/
		void deleteSet(const int& id);

		/*
		* 判断集合对象是否仍由本管理器持有（未被删除）
		* 用于外部缓存的裸指针在解引用前校验，防止使用已释放对象
		*/
		bool containsSet(const mSetBase* base) const;

		/*
		* 获取所有集合ID
		*/
		std::vector<int> getAllSetID(bool isContainAutoCreate = false);

		/*
		* 获取所有集合名称
		*/
		QStringList getAllSetNames(bool isContainAutoCreate = false);

		/*
		* 获取所有集合名称根据类型
		*/
		QStringList getAllSetNamesByTypes(MxFunctions::SetType type, bool isContainAutoCreate = false);
		/*
		* 返回创建的新ID
		*/
		int createID();


		/*
		* 判断集合被哪部分用到
		*/
		//因载荷边界统一了类型，做此修改
		bool beingUsed(const MxFunctions::SetType& settype, const MxFunctions::LoadBoundaryType& loadboundarytype);
		//bool beingUsed(const MxFunctions::SetType& settype, const MxFunctions::LoadBoundaryType& boundarytype);
		bool beingUsed(const MxFunctions::SetType& settype, const MxFunctions::ContactType& boundarytype);

		friend QDataStream& operator>>(QDataStream& in, mSetManager& mgr);
		friend QDataStream& InStreamSetManagerV3(QDataStream& in, mSetManager& mgr);
		friend QDataStream& operator<<(QDataStream& out, mSetManager& mgr);

	private:
		std::map<int, mSetBase*> _setData;

		int _maxID;
	};
}