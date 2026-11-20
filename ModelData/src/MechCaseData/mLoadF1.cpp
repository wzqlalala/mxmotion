#include "mLoadF1.h"

namespace MechCaseData
{
	mLoadF1::mLoadF1(int id, MxFunctions::LoadBoundaryType type, const QString& name)
		:mLoadBoundaryBase(id, type, name)
	{
		type_ = 0;
		loadname_ = "";
		delay_ = QVariant();
		xita_ = QVariant();
		curve1_ = "";
		curve2_ = "";
	}

	mLoadF1::~mLoadF1()
	{

	}

	std::shared_ptr<mLoadBoundaryBase> mLoadF1::copyLoadBoundary(const int& id, const QString& name)
	{
		std::shared_ptr<mLoadF1> load = std::make_shared<mLoadF1>(id, getLoadBoundaryType(), name);
		load->type_ = type_;
		load->delay_ = delay_;
		load->xita_ = xita_;
		load->curve1_ = curve1_;
		load->curve2_ = curve2_;
		return load;
	}

	MxFunctions::BrowserTreeStatus mLoadF1::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		if (loadname_.isEmpty())
		{
			info.append({ tip,"未选择载荷" });
		}
		if (curve1_.isEmpty())
		{
			info.append({ tip,"未选择幅值曲线" });
		}
		if ((type_==2||type_==3) && curve2_.isEmpty())
		{
			info.append({ tip,"未选择相位曲线" });
		}
		return info.isEmpty()?MxFunctions::BrowserTreeStatus::Tree_Yes: MxFunctions::BrowserTreeStatus::Tree_Failure;
	}
}