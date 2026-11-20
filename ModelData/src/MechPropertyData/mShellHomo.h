#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXPropertyData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>

#include "mPropertyBase.h"

namespace MechPropertyData
{
	//壳截面属性
	class MXPROPERTYDATA_EXPORT mShellHomo : public mPropertyBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mShellHomo(int id,MxFunctions::PropertyType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mShellHomo() {}

		/*
		* 析构函数
		*/
		~mShellHomo();

		/*
		* 拷贝分析工况
		*/
		mShellHomo* copyProperty(const int& id, const QString& name) override;

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

		friend QDataStream& operator>>(QDataStream& in, mShellHomo& mgr);
		friend QDataStream& InStreammShellHomoV3(QDataStream& in, mShellHomo& obj);
		friend QDataStream& operator<<(QDataStream& out, mShellHomo& mgr);

	public:

		//厚度
		QString _thickness;

		//单位面积非结构质量
		QString _nsm;

		//参考面位置(无/偏置距离/底部/顶部)，下拉框
		QString _referenceplane;

		//参考面偏置距离(参考面位置为偏置距离显示和可选，界面隐藏但不会对值进行处理)
		QString _z0;
	};
}
