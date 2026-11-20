#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>

#include "mLoadBoundaryBase.h"

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mBoundarySuperelement :public mLoadBoundaryBase
	{
	public:
		/*
		* 构造函数
		*/
		mBoundarySuperelement(int id, MxFunctions::LoadBoundaryType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mBoundarySuperelement() {}

		/*
		* 析构函数
		*/
		~mBoundarySuperelement();

		/*
		* 拷贝
		*/
		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;

		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info);

		friend QDataStream& operator>>(QDataStream& in, mBoundarySuperelement& obj);
		friend QDataStream& InStreammBoundarySuperelementV3(QDataStream& in, mBoundarySuperelement& obj);
		friend QDataStream& operator<<(QDataStream& out, mBoundarySuperelement& obj);

	public:
		QString _ConstraintType; //ASET\BNDFIX1\BNDFREE1
		QVector<bool> _DOF;
	};
}