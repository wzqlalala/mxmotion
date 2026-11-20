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
	class MXCASEDATA_EXPORT mLoadAcousticConcForce : public mLoadBoundaryBase
	{
	public:
		/*
		* 构造函数
		*/
		mLoadAcousticConcForce(int id, MxFunctions::LoadBoundaryType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mLoadAcousticConcForce() {}

		/*
		* 析构函数
		*/
		~mLoadAcousticConcForce();

		/*
		* 拷贝
		*/
		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;

		/*
		* 数据完整性判断
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mLoadAcousticConcForce& obj);
		friend QDataStream& InStreammLoadAcousticConcForceV3(QDataStream& in, mLoadAcousticConcForce& obj);
		friend QDataStream& operator<<(QDataStream& out, mLoadAcousticConcForce& obj);

	public:
		QVariant value_; //大小
	};
}