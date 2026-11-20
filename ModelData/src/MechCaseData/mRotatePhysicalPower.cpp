#include "mRotatePhysicalPower.h"
#include "mLoadBoundaryBase.h"

namespace MechCaseData
{
	mRotatePhysicalPower::mRotatePhysicalPower(int id, MxFunctions::LoadBoundaryType type, const QString& name)
		:mLoadBoundaryBase(id, type, name)
	{

	}

	mRotatePhysicalPower::~mRotatePhysicalPower()
	{

	}

	std::shared_ptr<mLoadBoundaryBase> mRotatePhysicalPower::copyLoadBoundary(const int& id, const QString& name)
	{
		std::shared_ptr<mRotatePhysicalPower> load = std::make_shared<mRotatePhysicalPower>(id, getLoadBoundaryType(), name);
		load->_startPoint1 = _startPoint1;
		load->_directPoint2 = _directPoint2;
		load->_rotateType = _rotateType;
		load->_defineType = _defineType;
		load->_uniformValue = _uniformValue;
		load->_elementCoorMap = _elementCoorMap;
		load->_dirPickState = _dirPickState;
		load->_picks = _picks;

		return load;
	}

	QString mRotatePhysicalPower::getSelectLoadType()
	{
		return QString("load");
	}

	MxFunctions::BrowserTreeStatus mRotatePhysicalPower::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}

}
