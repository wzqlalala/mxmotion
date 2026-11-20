#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QVector3D>
#include "mLoadBoundaryBase.h"

namespace MechCaseData
{

	class MXCASEDATA_EXPORT mLoadF1 : public mLoadBoundaryBase
	{
	public:
		/*
		* 构造函数
		*/
		mLoadF1(int id, MxFunctions::LoadBoundaryType type, const QString& name = "");
		mLoadF1() {};
		/*
		* 析构函数
		*/
		~mLoadF1();

		/*
		* 拷贝
		*/
		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;

		/*
		* 数据完整性判断
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mLoadF1& obj);
		friend QDataStream& InStreammLoadF1V3(QDataStream& in, mLoadF1& obj);
		friend QDataStream& operator<<(QDataStream& out, mLoadF1& obj);

	public:
		int type_;
		QString loadname_;
		QVariant delay_;
		QVariant xita_;

		QString curve1_;
		QString curve2_;
	};
}