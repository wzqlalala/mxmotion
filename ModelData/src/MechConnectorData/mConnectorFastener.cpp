#include "mConnectorFastener.h"

namespace MechConnectorData
{
	mConnectorFastener::mConnectorFastener(int id, MxFunctions::DeputyType type, const QString& name)
		: mDeputyBase(id, type, name)
	{
	}

	mConnectorFastener::~mConnectorFastener()
	{
	}

	std::shared_ptr<mDeputyBase> mConnectorFastener::copyDeputy(const int& id, const QString& name)
	{
		std::shared_ptr<mConnectorFastener> copy = std::make_shared<mConnectorFastener>(id, getDeputyType(), name);
		copy->defineType_ = defineType_;
		copy->nodeId_ = nodeId_;
		copy->tempNodeId_ = tempNodeId_;
		copy->influenceRadius_ = influenceRadius_;
		return copy;
	}

	MxFunctions::BrowserTreeStatus mConnectorFastener::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		QString tip = "警告";
		if (defineType_ == 0)
		{
			if (nodeId_ <= 0)
			{
				info.append({ tip, QString("节点未定义") });
				status = MxFunctions::BrowserTreeStatus::Tree_Failure;
			}
		}

		if (influenceRadius_ <= 0)
		{
			info.append({ tip,QString("影响半径未设置") });
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
		}
		return status;
	}
}
