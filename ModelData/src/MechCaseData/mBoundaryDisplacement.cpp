#include "mBoundaryDisplacement.h"

namespace MechCaseData
{
	mBoundaryDisplacement::mBoundaryDisplacement(int id,MxFunctions::LoadBoundaryType type, const QString& name)
		:mLoadBoundaryBase(id,type, name)
	{
		_definitionMode = 0;
		_U1 = QVariant();
		_U2 = QVariant();
		_U3 = QVariant();
		_UR1 = QVariant();
		_UR2 = QVariant();
		_UR3 = QVariant();
	}

	mBoundaryDisplacement::~mBoundaryDisplacement()
	{

	}

	std::shared_ptr<mLoadBoundaryBase> mBoundaryDisplacement::copyLoadBoundary(const int& id, const QString& name)
	{
		std::shared_ptr<mBoundaryDisplacement> boundary = std::make_shared<mBoundaryDisplacement>(id,getLoadBoundaryType(), name);
		boundary->_definitionMode = _definitionMode;
		boundary->_U1 = _U1;
		boundary->_U2 = _U2;
		boundary->_U3 = _U3;
		boundary->_UR1 = _UR1;
		boundary->_UR2 = _UR2;
		boundary->_UR3 = _UR3;
		boundary->_independentTable = _independentTable;
		return boundary;
	}
}