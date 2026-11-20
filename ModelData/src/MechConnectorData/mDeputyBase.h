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
	class MXCONNECTORDATA_EXPORT mDeputyBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mDeputyBase(int id, MxFunctions::DeputyType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mDeputyBase() {}

		/*
		* 析构函数
		*/
		virtual ~mDeputyBase();

		/*
		* 设置连接副ID
		*/
		void setDeputyId(int id);

		/*
		* 获取连接副ID
		*/
		int getDeputyId();

		/*
		* 设置连接副名
		*/
		void setDeputyName(const QString& name);

		/*
		* 获取连接副名
		*/
		QString getDeputyName();

		/*
		* 设置连接副类型
		*/
		void setDeputyType(MxFunctions::DeputyType type);

		/*
		* 获取连接英文
		*/
		QString getDeputyTypeToStringEn();

		/*
		* 获取连接副类型
		*/
		MxFunctions::DeputyType getDeputyType();

		/*
		* 拷贝连接副
		*/
		virtual std::shared_ptr<mDeputyBase> copyDeputy(const int& id, const QString& name) = 0;

		/*
		* 判断数据完整性
		*/
		virtual MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) = 0;

		/*
		* 获取连接副的所有节点
		*/
		virtual std::vector<int> getAllNodesInDeputy() { return std::vector<int>(); };

		/*
		* 设置抑制状态
		*/
		void setSuppressed(bool suppressed);

		/*
		* 获取抑制状态
		*/
		bool isSuppressed();

		friend QDataStream& operator>>(QDataStream& in, mDeputyBase& mgr);
		friend QDataStream& InStreammDeputyBaseV3(QDataStream& in, mDeputyBase& obj);
		friend QDataStream& operator<<(QDataStream& out, mDeputyBase& mgr);

	private:
		//连接副ID
		int _id;

		//连接副名字
		QString _deputyName;

		//连接副类型
		MxFunctions::DeputyType _deputyType;

		//是否被抑制
		bool _isSuppressed;
	};
}
