#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXConnectorData_global.h"
#include <map>
#include <QStringList>
#include <memory>
#include <vector>
#include "mPhysicalFieldEnum.h"
namespace MechConnectorData
{
	class mDeputyBase;
	class MXCONNECTORDATA_EXPORT mDeputyManager
	{
	public:
		mDeputyManager();

		~mDeputyManager();

		/*
		* 通过id获取连接副数据
		*/
		std::shared_ptr<mDeputyBase> getDeputyData(const int& id);

		/*
		* 通过名称获取连接副数据
		*/
		std::shared_ptr<mDeputyBase> getDeputyData(const QString& name);

		/*
		* 通过id获取名称
		*/
		QString getDeputyNameByID(const int& id);

		/*
		* 通过名称获取ID
		*/
		int getIDByDeputyName(const QString& name);

		/*
		* 添加连接副数据
		*/
		void AddDeputyData(std::shared_ptr<mDeputyBase> base);

		/*
		* 判断连接副名是否存在
		*/
		bool ExistDeputyName(const QString& name);

		/*
		* 重命名连接副
		*/
		void resetName(const QString& oldName, const QString& newName);

		/*
		* 拷贝连接副
		*/
		int copyDeputyData(const int& id);

		/*
		* 删除所有连接副
		*/
		void deleteAllDeputy();

		/*
		* 删除连接副
		*/
		void deleteDeputy(const int& id);

		/*
		* 获取连接副的所有节点
		*/
		std::vector<int> getAllNodesInDeputy(int id);
		std::vector<int> getAllNodesInDeputy();

		/*
		* 获取所有连接副ID
		*/
		std::vector<int> getAllDeputyID();

		/*
		* 获取所有连接副名称
		*/
		QStringList getAllDeputyNames();

		/*
		* 获取去重连接副名称
		*/
		QSet<QString> getAllDeputyNamesDup();

		/*
		* 获取所有连接副名称根据类型
		*/
		QStringList getAllDeputyNamesByTypes(MxFunctions::DeputyType type);

		/*
		* 通过类型枚举获取中文类型
		*/
		QString getDeputyTypeNameByType(MxFunctions::DeputyType type);

		/*
		* 返回创建的新ID
		*/
		int createID();

		friend QDataStream& operator>>(QDataStream& in, mDeputyManager& mgr);
		friend QDataStream& InStreammDeputyManagerV3(QDataStream& in, mDeputyManager& mgr);
		friend QDataStream& operator<<(QDataStream& out, mDeputyManager& mgr);

	private:
		std::map<int, std::shared_ptr<mDeputyBase>> _deputyData;

		int _maxID;
	};
}
