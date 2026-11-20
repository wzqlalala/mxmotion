#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXConnectorData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include "mConnectorBase.h"

namespace MechConnectorData
{
	class MXCONNECTORDATA_EXPORT mConnectorBind : public mConnectorBase
	{
	public:
		/*
		* 构造函数
		*/
		mConnectorBind(int id, MxFunctions::ConnectorType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mConnectorBind() {}

		/*
		* 析构函数
		*/
		~mConnectorBind();

		/*
		* 拷贝
		*/
		std::shared_ptr<mConnectorBase> copyConnector(const int& id, const QString& name) override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		/*
		* 设置，获取拾取状态
		*/
		void setPickStateMaster(int pickstate);
		int getPickStateMaster();
		void setPickStateSlave(int pickstate);
		int getPickStateSlave();

		friend QDataStream& operator>>(QDataStream& in, mConnectorBind& mgr);
		friend QDataStream& InStreammConnectorBindV3(QDataStream& in, mConnectorBind& obj);
		friend QDataStream& operator<<(QDataStream& out, mConnectorBind& mgr);

	private:
		//具有通用拾取控件的面板 需要设置此参数 0表示“无” 1表示“无临时数据” 2表示“有临时数据”
		int _pickStateMaster;   //主面
		int _pickStateSlave;	//从面
	public:
		
		//类型
		int type_;

		//容差
		int allowrancetype_;
		QVariant allowrance_;

		//调整
		bool adjust_;
	};
}