#include "mBoundaryTempNew.h"

namespace MechCaseData
{
	mBoundaryTempNew::mBoundaryTempNew(int id, MxFunctions::LoadBoundaryType type, const QString& name)
		:mLoadBoundaryBase(id, type, name)
	{
	}

	mBoundaryTempNew::~mBoundaryTempNew()
	{

	}

	std::shared_ptr<mLoadBoundaryBase> mBoundaryTempNew::copyLoadBoundary(const int& id, const QString& name)
	{
		std::shared_ptr<mBoundaryTempNew> boundary = std::make_shared<mBoundaryTempNew>(id, getLoadBoundaryType(), name);
		boundary->_mainModelFile = _mainModelFile;
		boundary->_timeStep = _timeStep;
		boundary->_mainSet = _mainSet;
		boundary->_useMainSet = _useMainSet;
		boundary->_pickData = _pickData;
		return boundary;
	}

	MxFunctions::BrowserTreeStatus mBoundaryTempNew::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		if (_mainModelFile.isEmpty())
		{
			info.append({ tip,"文件未设置" });
		}
		if (_timeStep.isEmpty())
		{
			info.append({ tip,"时间步未设置" });
		}
		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}
}
