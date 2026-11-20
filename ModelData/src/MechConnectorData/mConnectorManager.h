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
	class mConnectorBase;
	class MXCONNECTORDATA_EXPORT mConnectorManager
	{
	public:
		mConnectorManager();

		~mConnectorManager();

		/*
		* 通过id获取连接数据
		*/
		std::shared_ptr<mConnectorBase> getConnectorData(const int& id);

		/*
		* 通过名称获取连接数据
		*/
		std::shared_ptr<mConnectorBase> getConnectorData(const QString& name);

		/*
		* 通过id获取名称
		*/
		QString getConnectorNameByID(const int& id);

		/*
		* 通过名称获取ID
		*/
		int getIDByConnectorName(const QString& name);

		/*
		* 添加连接数据
		*/
		void AddConnectorData(std::shared_ptr<mConnectorBase> base);

		/*
		* 判断连接名是否存在
		*/
		bool ExistConnectorName(const QString& name);

		/*
		* 重命名连接
		*/
		void resetName(const QString& oldName, const QString& newName);

		/*
		* 拷贝连接
		*/
		int copyConnectorData(const int& id);

		/*
		* 删除所有连接
		*/
		void deleteAllConnector();

		/*
		* 删除连接
		*/
		void deleteConnector(const int& id);

		/*
		* 获取连接的所有节点
		*/
		std::vector<int>getAllNodesInConn(int id);
		std::vector<int>getAllNodesInConn();

		/*
		* 获取所有连接ID
		*/
		std::vector<int> getAllConnectorID();

		/*
		* 获取所有连接名称
		*/
		QStringList getAllConnectorNames();

		/*
		* 获取去重连接名称
		*/
		QSet<QString>getAllConnectorNamesDup();

		/*
		* 获取所有连接名称根据类型
		*/
		QStringList getAllConnectorNamesByTypes(MxFunctions::ConnectorType type);

		/*
		* 通过类型枚举获取中文类型
		*/
		QString getConnectorTypeNameByType(MxFunctions::ConnectorType type);

		/*
		* 返回创建的新ID
		*/
		int createID();

		friend QDataStream& operator>>(QDataStream& in, mConnectorManager& mgr);
		friend QDataStream& InStreammConnectorManagerV3(QDataStream& in, mConnectorManager& mgr);
		friend QDataStream& operator<<(QDataStream& out, mConnectorManager& mgr);

	private:
		std::map<int, std::shared_ptr<mConnectorBase>> _connectorData;

		int _maxID;
	};
}