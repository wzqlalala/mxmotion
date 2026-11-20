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
	class MXCASEDATA_EXPORT mLoadBolt : public mLoadBoundaryBase
	{
	
	public:
		/*
		* 构造函数
		*/
		mLoadBolt(int id, MxFunctions::LoadBoundaryType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mLoadBolt() {}

		/*
		* 析构函数
		*/
		~mLoadBolt();

		/*
		* 拷贝
		*/
		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;

		/*
		* 数据完整性判断
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mLoadBolt& obj);
		friend QDataStream& InStreammLoadBoltV3(QDataStream& in, mLoadBolt& obj);
		friend QDataStream& operator<<(QDataStream& out, mLoadBolt& obj);

	public:
		//方式 0力预紧 1位移预紧 2螺栓锁止
		int way_;
		//参数
		QVariant value_;
	};
}