#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXConnectorData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QVector>
#include "mDeputyBase.h"

namespace MechConnectorData
{
	class MXCONNECTORDATA_EXPORT mConnectingDeputy : public mDeputyBase
	{
	public:
		/*
		* 构造函数
		*/
		mConnectingDeputy(int id, MxFunctions::DeputyType type, const QString& name = "");


		mConnectingDeputy(){}
		/*
		* 析构函数
		*/
		~mConnectingDeputy();

		/*
		* 拷贝
		*/
		std::shared_ptr<mDeputyBase> copyDeputy(const int& id, const QString& name) override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mConnectingDeputy& mgr);
		friend QDataStream& InStreammConnectingDeputyV3(QDataStream& in, mConnectingDeputy& obj);
		friend QDataStream& operator<<(QDataStream& out, mConnectingDeputy& mgr);


	public:
		//参考位置节点ID列表（与表格行一一对应）
		QVector<int> refNodeIds_;

		//移动位置节点ID列表（与表格行一一对应）
		QVector<int> movNodeIds_;

		//类型
		QString datatype_;

		//子类型（球铰/滑动下的旋转/对齐/万向节）
		QString subdatatype_;
	};
}