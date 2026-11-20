#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXPropertyData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>

#include "mPropertyBase.h"

namespace MechPropertyData
{
	//宏观等效截面属性
	class MXPROPERTYDATA_EXPORT mMacroEquivalent : public mPropertyBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mMacroEquivalent(int id, MxFunctions::PropertyType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mMacroEquivalent() {}

		/*
		* 析构函数
		*/
		~mMacroEquivalent();

		/*
		* 拷贝分析属性
		*/
		mMacroEquivalent* copyProperty(const int& id, const QString& name) override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mMacroEquivalent& obj);
		friend QDataStream& InStreammMacroEquivalentV3(QDataStream& in, mMacroEquivalent& obj);
		friend QDataStream& operator<<(QDataStream& out, mMacroEquivalent& obj);


	public:
		//间隙距离
		QString _frbName;
	};
}
