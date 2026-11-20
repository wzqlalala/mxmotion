#include "mConnectingDeputy.h"

namespace MechConnectorData
{
	mConnectingDeputy::mConnectingDeputy(int id, MxFunctions::DeputyType type, const QString& name)
		:mDeputyBase(id, type, name)
	{
		datatype_ = "球铰";
		subdatatype_ = "旋转";
	}

	mConnectingDeputy::~mConnectingDeputy()
	{

	}

	std::shared_ptr<mDeputyBase> mConnectingDeputy::copyDeputy(const int& id, const QString& name)
	{
		std::shared_ptr<mConnectingDeputy> conn = std::make_shared<mConnectingDeputy>(id, getDeputyType(), name);
		conn->refNodeIds_ = refNodeIds_;
		conn->movNodeIds_ = movNodeIds_;
		conn->datatype_ = datatype_;
		conn->subdatatype_ = subdatatype_;
		return conn;
	}

	MxFunctions::BrowserTreeStatus mConnectingDeputy::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		QString tip = "警告";
		if (refNodeIds_.size() == 0)
		{
			info.append({ tip,"参考位置未设置" });
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
		}
		for (int i = 0; i < refNodeIds_.size(); ++i)
		{
			if (refNodeIds_[i] <= 0)
			{
				info.append({ tip,QString("第%1行参考位置未设置").arg(i + 1) });
				status = MxFunctions::BrowserTreeStatus::Tree_Failure;
			}
		}
		return status;
	}
}