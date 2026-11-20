#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXConnectorData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mModelEnum.h"

#include <QString>
#include <QSet>
#include <QHash>
#include <QVariant>
#include <QStringList>
#include <memory>

//MxFunction
#include "mPhysicalFieldEnum.h"
#include "mElementFunction.h"
#include "mModelStruct.h"

namespace MechConnectorData
{
	class MXCONNECTORDATA_EXPORT mConnectorBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mConnectorBase(int id, MxFunctions::ConnectorType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mConnectorBase() {}

		/*
		* 析构函数
		*/
		virtual ~mConnectorBase();


		/*
		* 设置连接ID
		*/
		void setConnectorId(int id);

		/*
		* 获取材料ID
		*/
		int getConnectorId();

		/*
		* 设置连接名
		*/
		void setConnectorName(const QString& name);

		/*
		* 获取连接名
		*/
		QString getConnectorName();

		/*
		* 设置连接类型
		*/
		void setConnectorType(MxFunctions::ConnectorType type);

		/*
		* 获取连接类型
		*/
		MxFunctions::ConnectorType getConnectorType();

		/*
		* 获取连接英文
		*/
		QString getConnectorTypeToStringEn();

		/*
		* 拷贝连接
		*/
		virtual std::shared_ptr<mConnectorBase> copyConnector(const int& id, const QString& name) = 0;

		/*
		* 判断数据完整性
		*/
		virtual MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) = 0;

		/*
		* 获取连接的所有节点
		*/
		virtual std::vector<int> getAllNodesInConnector() { return std::vector<int>(); };

		/*
		* 判断是否为连接副
		*/
		virtual bool isDeputy() const { return false; }

		/*
		* 设置抑制状态
		*/
		void setSuppressed(bool suppressed);

		/*
		* 获取抑制状态
		*/
		bool isSuppressed();

		friend QDataStream& operator>>(QDataStream& in, mConnectorBase& mgr);
		friend QDataStream& InStreammConnectorBaseV3(QDataStream& in, mConnectorBase& obj);
		friend QDataStream& operator<<(QDataStream& out, mConnectorBase& mgr);

	private:
		//连接ID
		int _id;

		//连接名字
		QString _connectorName;

		//连接类型
		MxFunctions::ConnectorType _connectorType;

		//用户重命名
		bool userRenamed_;

		//是否被抑制
		bool _isSuppressed;
	};
}