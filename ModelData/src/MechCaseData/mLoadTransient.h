#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
//#include "mLoadBase.h"
#include "mLoadBoundaryBase.h"

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mLoadTransient : public mLoadBoundaryBase
	{
	public:
		/*
		* 构造函数
		*/
		mLoadTransient(int id, MxFunctions::LoadBoundaryType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mLoadTransient() {}

		/*
		* 析构函数
		*/
		~mLoadTransient();

		/*
		* 拷贝
		*/
		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;

		/*
		* 获取当前所选载荷类型
		*/
		QString getSelectLoadType();

		/*
		* 数据完整性判断
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mLoadTransient& obj);
		friend QDataStream& InStreammLoadTransientV3(QDataStream& in, mLoadTransient& obj);
		friend QDataStream& operator<<(QDataStream& out, mLoadTransient& obj);

	public:
		QVariant pa_;
		int way_ = 0; //载荷类型 0力 1位移 2速度 3加速度
	};
}