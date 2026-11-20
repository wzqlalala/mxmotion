#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QList>
#include "mLoadBoundaryBase.h"

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mLoadCombined: public mLoadBoundaryBase
	{
	public:
		/*
		* 构造函数
		*/
		mLoadCombined(int id, MxFunctions::LoadBoundaryType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mLoadCombined() {}

		/*
		* 析构函数
		*/
		~mLoadCombined();

		/*
		* 拷贝
		*/
		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;

		/*
		* 数据完整性判断
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mLoadCombined& obj);
		friend QDataStream& InStreammLoadCombinedV3(QDataStream& in, mLoadCombined& obj);
		friend QDataStream& operator<<(QDataStream& out, mLoadCombined& obj);

	public:
		QVariant ktotal_;
		QList<double> ratioList_;
	};
}