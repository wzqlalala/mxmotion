#include "mCurvePropPageBase.h"
#include "../CurveCore/mCurveDocController.h"

#include <QPalette>

namespace PostCurve
{
	mCurvePropPageBase::mCurvePropPageBase(QWidget* parent)
		: mPanelBase(parent)
	{
		setAutoFillBackground(true);
		//面板底色（设计图扁平白底；用 palette 避免样式表向子控件级联）。
		//控件状态样式由基类 mPanelBase 构造统一应用（三方参数栏同源）
		QPalette pal = palette();
		pal.setColor(QPalette::Window, QColor(0xFF, 0xFF, 0xFF));
		setPalette(pal);
		//模型变化（含撤销/重做/其他视图修改）-> 回填
		connect(mCurveDocController::getInstance(), &mCurveDocController::sig_nodePropertyChanged,
			this, [this](int nodeId, const QString& prop)
		{
			onNodePropertyChanged(nodeId, prop);
		});
	}

	int mCurvePropPageBase::nodeId() const
	{
		return _nodeId;
	}

	void mCurvePropPageBase::setNodeId(int nodeId)
	{
		_nodeId = nodeId;
		_loading = true;
		loadFromModel();
		_loading = false;
	}

	void mCurvePropPageBase::commit(const QString& prop, const QVariant& value)
	{
		if (_loading || _nodeId < 0)
			return;
		mCurveDocController::getInstance()->setNodeProperty(_nodeId, prop, value);
	}

	void mCurvePropPageBase::onNodePropertyChanged(int nodeId, const QString& prop)
	{
		Q_UNUSED(prop)
		if (nodeId != _nodeId)
			return;
		if (_loading)
			return;
		_loading = true;
		loadFromModel();
		_loading = false;
	}
}
