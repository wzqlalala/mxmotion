#include "mEleSetCtrl.h"

namespace MechModelFixData
{
	mEleSetCtrl::mEleSetCtrl(int id, const QString& name)
		: mModelFixDataBase(id, name)
	{
		if (name.isEmpty())
		{
			_modelFixName = QString("ModelChang_%1").arg(_id);
		}
	}

	mEleSetCtrl::~mEleSetCtrl()
	{
	}

	std::shared_ptr<mModelFixDataBase> mEleSetCtrl::copyModelFixData(const int& id, const QString& name)
	{
		std::shared_ptr<mEleSetCtrl> eleSetCtrl = std::make_shared<mEleSetCtrl>(id, name);
		eleSetCtrl->_activeOrRemove = _activeOrRemove;
		eleSetCtrl->_selectSetType = _selectSetType;
		eleSetCtrl->setSuppressed(false);
		return eleSetCtrl;
	}

	MxFunctions::BrowserTreeStatus mEleSetCtrl::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		return info.isEmpty() ? MxFunctions::Tree_Yes : MxFunctions::Tree_Failure;
	}

	QString mEleSetCtrl::getModelFixType() const
	{
		return "EleSetCtrl";
	}

	void mEleSetCtrl::setActiveOrRemove(bool activeOrRemove)
	{
		_activeOrRemove = activeOrRemove;
	}

	bool mEleSetCtrl::isActiveOrRemove() const
	{
		return _activeOrRemove;
	}

	void mEleSetCtrl::setSelectSetType(const QString& type)
	{
		_selectSetType = type;
	}

	QString mEleSetCtrl::getSelectSetType() const
	{
		return _selectSetType;
	}
}
