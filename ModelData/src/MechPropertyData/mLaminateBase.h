#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXPropertyData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mModelEnum.h"
#include <QString>
#include <QPair>

namespace MechPropertyData
{
	class MXPROPERTYDATA_EXPORT mLaminateBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mLaminateBase(int id, MxFunctions::LaminateType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mLaminateBase() {}

		/*
		* 析构函数
		*/
		virtual ~mLaminateBase();

		/*
		* 设置ID
		*/
		void setLaminateId(int id);

		/*
		* 获取ID
		*/
		int getLaminateId();

		/*
		* 设置名称
		*/
		void setLaminateName(const QString& name);

		/*
		* 获取名称
		*/
		QString getLaminateName();

		/*
		* 设置属性类型
		*/
		void setLaminateType(MxFunctions::LaminateType type);

		/*
		* 获取属性类型
		*/
		MxFunctions::LaminateType getLaminateType();

		/*
		* 获取属性英文
		*/
		QString laminateTypeToStringEn();


		/*
		* 获取属性类型的字符串
		*/
		QString getLaminateTypeStr();

		/*
		* 拷贝分析属性
		*/
		virtual mLaminateBase* copyLaminate(const int& id, const QString& name) = 0;

		/*
		* 判断数据完整性
		*/
		virtual MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) { return MxFunctions::BrowserTreeStatus::Tree_Yes; };

		friend QDataStream& operator>>(QDataStream& in, mLaminateBase& mgr);
		friend QDataStream& InStreammLaminateBaseV3(QDataStream& in, mLaminateBase& obj);
		friend QDataStream& operator<<(QDataStream& out, mLaminateBase& mgr);

	private:

		//ID
		int _id;

		//名字
		QString _LaminateName;

		//类型
		MxFunctions::LaminateType _LaminateType;
	};
}