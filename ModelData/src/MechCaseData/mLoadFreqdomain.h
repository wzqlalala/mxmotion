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
	class MXCASEDATA_EXPORT mLoadFreqdomain : public mLoadBoundaryBase
	{
	public:
		/*
		* 构造函数
		*/
		mLoadFreqdomain(int id, MxFunctions::LoadBoundaryType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mLoadFreqdomain() {}

		/*
		* 析构函数
		*/
		~mLoadFreqdomain();

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

		friend QDataStream& operator>>(QDataStream& in, mLoadFreqdomain& obj);
		friend QDataStream& InStreammLoadFreqdomainV3(QDataStream& in, mLoadFreqdomain& obj);
		friend QDataStream& operator<<(QDataStream& out, mLoadFreqdomain& obj);

	public:
		QVariant delay_;
		QVariant phase_;
		int way_ = 0;  //载荷类型 0力 1位移 2速度 3加速度
		int way2_ = 0; //频域载荷类型 0=rload 1=rload1
	};
}