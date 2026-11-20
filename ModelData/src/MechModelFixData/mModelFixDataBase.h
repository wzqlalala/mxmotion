#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXModelFixData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mModelEnum.h"

#include <QString>
#include <QVector>
#include <QPair>
#include <memory>

//MxFunction
#include "mPhysicalFieldEnum.h"
#include "mElementFunction.h"
#include "mModelStruct.h"

namespace MechModelFixData
{
	class MXMODELFIXDATA_EXPORT mModelFixDataBase
	{
	public:
		/*
		* 含参构造函数
		*/
		mModelFixDataBase(int id, const QString& name = "");

		/*
		* 默认构造函数，仅用于输入输出流
		*/
		mModelFixDataBase() {}

		/*
		* 析构函数
		*/
		virtual ~mModelFixDataBase();

		/*
		* 设置ID
		*/
		void setId(int id);
		int getId() const;

		/*
		* 名称
		*/
		void setModelFixName(const QString& name);
		QString getModelFixName() const;

		/*
		* 设置抑制状态
		*/
		void setSuppressed(bool suppressed);

		/*
		* 获取抑制状态
		*/
		bool isSuppressed() const;

		/*
		* 拷贝模型修复数据
		*/
		virtual std::shared_ptr<mModelFixDataBase> copyModelFixData(const int& id, const QString& name) = 0;

		/*
		* 判断数据完整性
		*/
		virtual MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) = 0;

		/*
		* 获取模型修复类型字符串
		*/
		virtual QString getModelFixType() const = 0;

		friend QDataStream& operator>>(QDataStream& in, mModelFixDataBase& obj);
		friend QDataStream& InStreammModelFixDataBaseV3(QDataStream& in, mModelFixDataBase& obj);
		friend QDataStream& operator<<(QDataStream& out, mModelFixDataBase& obj);

	protected:
		//模型修复ID
		int _id;

		//模型修复名称
		QString _modelFixName;

		//抑制状态
		bool _isSuppressed;
	};
}
