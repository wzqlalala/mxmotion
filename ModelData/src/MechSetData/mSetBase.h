#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <QPair>
#include <vector>
#include <unordered_map>
#include <QDataStream>

#include "MXSetData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mModelEnum.h"


namespace MechSetData
{
	class MXSETDATA_EXPORT mSetBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mSetBase(int id,MxFunctions::SetType type, const QString& name = "",bool internal = false, bool isUserRename = false);

		/*
		* 仅用于输入输出流
		*/
		mSetBase() {}

		/*
		* 析构函数
		*/
		virtual ~mSetBase();

		/*
		* 获取是否为用户重命名
		*/
		bool getIsUserRename();

		/*
		* 获取是否为内部集合
		*/
		bool getInternal();

		/*
		* 设置为用户定义
		*/
		void setInternal(bool isinter);

		/*
		* 设置集合ID
		*/
		void setSetId(int id);

		/*
		* 获取集合ID
		*/
		int getSetId();

		/*
		* 设置集合名
		*/
		void setSetName(const QString& name);

		/*
		* 获取集合名
		*/
		QString getSetName();

		/*
		* 设置集合类型
		*/
		void setSetType(MxFunctions::SetType type);
		/*
		* 获取集合类型
		*/
		MxFunctions::SetType getSetType();

		/*
		* 获取集合类型的字符串
		*/
		QString getSetTypeStr();

		/*
		* 拷贝分析集合
		*/
		virtual mSetBase* copySet(const int& id, const QString& name) = 0;

		/*
		* 判断数据完整性
		*/
		virtual MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) = 0;

		friend QDataStream& operator>>(QDataStream& in, mSetBase& obj);
		friend QDataStream& InStreamSetBaseV3(QDataStream& in, mSetBase& obj);
		friend QDataStream& operator<<(QDataStream& out, mSetBase& obj);

	private:

		//集合ID
		int _id;

		//集合名字
		QString _setName;

		//集合类型
		MxFunctions::SetType _setType;

		//内部集合
		bool _internal;

		//是否为用户重命名
		bool _isUserRename;
	};
}
