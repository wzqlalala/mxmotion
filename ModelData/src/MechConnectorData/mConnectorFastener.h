#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXConnectorData_global.h"
#include "mDeputyBase.h"
#include <QString>
#include <QVector>
#include <memory>

namespace MechConnectorData
{
	class MXCONNECTORDATA_EXPORT mConnectorFastener : public mDeputyBase
	{
	public:
		mConnectorFastener(int id, MxFunctions::DeputyType type, const QString& name = "");

		mConnectorFastener(){}

		~mConnectorFastener();

		std::shared_ptr<mDeputyBase> copyDeputy(const int& id, const QString& name) override;

		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mConnectorFastener& mgr);
		friend QDataStream& InStreammConnectorFastenerV3(QDataStream& in, mConnectorFastener& obj);
		friend QDataStream& operator<<(QDataStream& out, mConnectorFastener& mgr);

	public:
		// 0:节点定义, 1:连接器部件定义
		int defineType_ = 0;
		// 节点定义时：存储节点ID
		int nodeId_ = -1;
		// 节点定义时：临时节点ID（与选中节点同坐标）
		int tempNodeId_ = -1;
		// 影响半径
		double influenceRadius_ = 0.0;
	};
}
