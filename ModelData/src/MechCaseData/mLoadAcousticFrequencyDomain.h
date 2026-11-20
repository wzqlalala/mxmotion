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
	class MXCASEDATA_EXPORT mLoadAcousticFrequencyDomain : public mLoadBoundaryBase
	{
	public:
		/*
		* 构造函数
		*/
		mLoadAcousticFrequencyDomain(int id, MxFunctions::LoadBoundaryType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mLoadAcousticFrequencyDomain() {}

		/*
		* 析构函数
		*/
		~mLoadAcousticFrequencyDomain();

		/*
		* 拷贝
		*/
		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;

		/*
		* 数据完整性判断
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mLoadAcousticFrequencyDomain& obj);
		friend QDataStream& InStreammLoadAcousticFrequencyDomainV3(QDataStream& in, mLoadAcousticFrequencyDomain& obj);
		friend QDataStream& operator<<(QDataStream& out, mLoadAcousticFrequencyDomain& obj);

	public:
		QVariant delay_; //延迟时间
		QVariant xita_; //相位
		QVariant rho_;	//声场密度
		QVariant B_;	//体积模量
	};
}