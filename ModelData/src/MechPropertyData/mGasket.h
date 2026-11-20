#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXPropertyData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>

#include "mPropertyBase.h"

namespace MechPropertyData
{
	//垫片截面属性
	class MXPROPERTYDATA_EXPORT mGasket : public mPropertyBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mGasket(int id, MxFunctions::PropertyType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mGasket() {}

		/*
		* 析构函数
		*/
		~mGasket();

		/*
		* 拷贝分析工况
		*/
	    mGasket* copyProperty(const int& id, const QString& name) override;

		/*
		* 设置厚度
		*/
		void setThickness(QString thickness) override;

		/*
		* 获取厚度
		*/
		QString getThickness() override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mGasket& mgr);
		friend QDataStream& InStreammGasketV3(QDataStream& in, mGasket& obj);
		friend QDataStream& operator<<(QDataStream& out, mGasket& mgr);

	public:

		//厚度定义（自动计算/指定厚度）
		QString _thicknessDef;

		//初始厚度
		QString _initialThickness;

		//初始间隙
		QString _initialGap;
	};
}
