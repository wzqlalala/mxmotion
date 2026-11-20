#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXConnectorData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include "mConnectorBase.h"
#include <map>
namespace MechConnectorData
{
	class MXCONNECTORDATA_EXPORT mConnectorBearingUnits : public mConnectorBase
	{
	public:
		/*
		* 构造函数
		*/
		mConnectorBearingUnits(int id, MxFunctions::ConnectorType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mConnectorBearingUnits() {}

		/*
		* 析构函数
		*/
		~mConnectorBearingUnits();

		/*
		* 拷贝
		*/
		std::shared_ptr<mConnectorBase> copyConnector(const int& id, const QString& name) override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		/*
		* 获取连接的所有节点
		*/
		std::vector<int> getAllNodesInConnector()override;

		friend QDataStream& operator>>(QDataStream& in, mConnectorBearingUnits& mgr);
		friend QDataStream& InStreammConnectorBearingUnitsV3(QDataStream& in, mConnectorBearingUnits& obj);
		friend QDataStream& operator<<(QDataStream& out, mConnectorBearingUnits& mgr);
	public:
		//类型
		int type_;

		//节点1
		std::pair<MxFunctions::PickDataType, RenderPickResultData> node1_;

		//节点2
		std::pair<MxFunctions::PickDataType, RenderPickResultData> node2_;

		//旋转平面
		int rotateFace_;

		//定义方式
		int defMode_;

		//轴承参数定义(8个)
		std::vector<double> bearingPara_;

		//随转速变化曲线
		QVector<QVector<double>> curveRotate_;
	};
}
