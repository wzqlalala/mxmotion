#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXMaterialData_global.h"
#include <map>
#include <QStringList>
#include <vector>
#include "mPhysicalFieldEnum.h"
#include "mUnitEnum.h"
#include "mUnitSystemManager.h"
#include <memory>

using namespace CommonData;
namespace MechMaterialData
{
	class mMaterialBase;
	class MXMATERIALDATA_EXPORT mMaterialManager
	{
	public:
		mMaterialManager();

		~mMaterialManager();
	
		/*
		* 通过id获取材料数据
		*/
		std::shared_ptr<mMaterialBase> getMaterialData(const int& id);

		/*
		* 通过名称获取材料数据
		*/
		std::shared_ptr<mMaterialBase>getMaterialData(const QString& name);

		/*
		* 通过id获取名称
		*/
		QString getMatNameByID(int id);

		/*
		* 通过名称获取id
		*/
		int getIDByMatName(QString name);

		/*
		* 添加材料数据
		*/
		void AddMaterialData(mMaterialBase* mat);
		void AddMaterialData(std::shared_ptr<mMaterialBase>);

		/*
		* 判断材料名是否存在
		*/
		bool ExistMaterialName(const QString& name);

		/*
		* 重命名材料
		*/
		void resetName(const QString& oldName, const QString& newName);

		/*
		* 拷贝材料
		*/
		int copyMaterialData(const int& id);

		/*
		* 删除所有材料
		*/
		void deleteAllMat();

		/*
		* 删除材料
		*/
		void deleteMat(const int& id);

		/*
		* 获取所有材料ID
		*/
		std::vector<int> getAllMatID();

		/*
		* 获取所有材料名称
		*/
		QStringList getAllMatNames();

		/*
		* 根据类型获取所有材料名称
		*/
		QStringList getAllMatNamesByTypes(MxFunctions::MaterialType type);

		/*
		* 返回创建的新ID
		*/
		int createID();

		/*
		* 响应单位制转换
		*/
		void responseUnitChanged(const int& unit,mUnitSystemManager*unitsystemmanager);

		friend QDataStream& operator>>(QDataStream& in, mMaterialManager& mgr);
		friend QDataStream& InStreammMaterialManagerV3(QDataStream& in, mMaterialManager& mgr);
		friend QDataStream& operator<<(QDataStream& out, mMaterialManager& mgr);

	private:
		std::map<int, std::shared_ptr<mMaterialBase>> _materialData;

		int _maxID;
	};
}