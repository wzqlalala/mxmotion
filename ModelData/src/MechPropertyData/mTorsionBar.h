#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXPropertyData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>

#include "mPropertyBase.h"

namespace MechPropertyData
{
	//杆截面属性
	class MXPROPERTYDATA_EXPORT mTorsionBar : public mPropertyBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mTorsionBar(int id,MxFunctions::PropertyType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mTorsionBar() {}

		/*
		* 析构函数
		*/
		~mTorsionBar();

		/*
		* 拷贝分析工况
		*/
	    mTorsionBar* copyProperty(const int& id, const QString& name) override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mTorsionBar& mgr);
		friend QDataStream& InStreammTorsionBarV3(QDataStream& in, mTorsionBar& obj);
		friend QDataStream& operator<<(QDataStream& out, mTorsionBar& mgr);

	public:

		//扭转矩
		QString _J;

		//梁的横截面积
		QString _A;

		//单位长度的非结构化质量
		QString _nsm;
	};
}
