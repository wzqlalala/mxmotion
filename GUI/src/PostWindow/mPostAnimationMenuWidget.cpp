#include "mPostAnimationMenuWidget.h"

//Post
#include "mPostDataBase.h"
#include "mPostDataManager.h"
#include "mPostOneFrameRendData.h"

#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mPostRigidWallData.h"
#include "mxr/view/REBUILD_PostModelRender.h"

//PreData
#include "mViewportModel.h"
#include "mViewportModelManager.h"

//MechCommon;
#include "mMechGlobalSignals.h"

//CustomTooltip
#include "mCustomTooltip.h"
#include "mOtherFunction.h"

using namespace MechPreData;
using namespace PostData;
using namespace MechCommon;

namespace PostWindow
{
	PostWindow::mPostAnimationMenuWidget::mPostAnimationMenuWidget(QWidget* parent)
		: QWidget(parent)
	{
		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::setAnimationMenuStatus, this, &mPostAnimationMenuWidget::setAnimationMenuStatus);
		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updateAnimationMenuGUI, this, &mPostAnimationMenuWidget::updateAnimationMenuGUI);
		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::pauseAnimation, this, &mPostAnimationMenuWidget::pauseAnimation);
		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updateAnimationHideFailedElementGUI, this, &mPostAnimationMenuWidget::setHideFailedElement);

	}

	void mPostAnimationMenuWidget::setAnimationMenuStatus(bool isGenerate)
	{
		if (isGenerate)
		{
			_btnPlay->setEnabled(isGenerate);
			_btnExportVideo->setEnabled(isGenerate);
			_btnCustomVideo->setEnabled(isGenerate);
			if (_checkHideFailedElement != nullptr)
				_checkHideFailedElement->setEnabled(isGenerate);
		}
		else
		{
			_btnFirst->setEnabled(isGenerate);
			_btnPrevious->setEnabled(isGenerate);
			_btnNext->setEnabled(isGenerate);
			_btnLast->setEnabled(isGenerate);
			_btnPlay->setEnabled(isGenerate);
			_comboRange->setEnabled(isGenerate);
			_checkLoopPlay->setEnabled(isGenerate);
			_sliderRate->setEnabled(isGenerate);
			_lineEditRate->setEnabled(isGenerate);
			_btnExportVideo->setEnabled(isGenerate);
			_btnCustomVideo->setEnabled(isGenerate);
			if (_checkHideFailedElement != nullptr)
				_checkHideFailedElement->setEnabled(isGenerate);
		}
	}

	void mPostAnimationMenuWidget::updateAnimationMenuGUI(QString view)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(view);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		QString postname = postRend->getPostDataName();
		mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
		if (postDB == nullptr)
			return;

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr)
			return;

		_btnPlay->blockSignals(true);
		_btnFirst->blockSignals(true);
		_btnPrevious->blockSignals(true);
		_btnNext->blockSignals(true);
		_btnLast->blockSignals(true);
		_comboRange->blockSignals(true);
		_checkLoopPlay->blockSignals(true);
		_sliderRate->blockSignals(true);
		_lineEditRate->blockSignals(true);
		if (_checkHideFailedElement != nullptr)
			_checkHideFailedElement->blockSignals(true);

		if (guidata->_generateAnimate)
		{
			if (guidata->_aniIsStart)
			{
				_btnPlay->setChecked(true);
				_btnPlay->setIcon(QIcon(":/mMainWidgets/Icon/post/zanting.png"));
				_btnPlay->setText(QString("暂停"));
				_btnPlay->installEventFilter(new TooltipInstaller("暂停",
					QPixmap(":/mMainWidgets/Icon/post/zanting.png"),
					"暂停播放动画。", 200, _btnPlay));
				_btnFirst->setEnabled(false);
				_btnPrevious->setEnabled(false);
				_btnNext->setEnabled(false);
				_btnLast->setEnabled(false);
				_btnPlay->setEnabled(true);
				_comboRange->setEnabled(false);
				_checkLoopPlay->setEnabled(false);
				_sliderRate->setEnabled(false);
				_lineEditRate->setEnabled(false);
				if (_checkHideFailedElement != nullptr)
					_checkHideFailedElement->setEnabled(false);
			}
			else
			{
				_btnPlay->setChecked(false);
				_btnPlay->setIcon(QIcon(":/mMainWidgets/Icon/post/kaishi.png"));
				_btnPlay->setText(QString("开始"));
				_btnPlay->installEventFilter(new TooltipInstaller("开始",
					QPixmap(":/mMainWidgets/Icon/post/zanting.png"),
					"开始播放动画。", 200, _btnPlay));
				_btnFirst->setEnabled(true);
				_btnPrevious->setEnabled(true);
				_btnNext->setEnabled(true);
				_btnLast->setEnabled(true);
				_btnPlay->setEnabled(true);
				_comboRange->setEnabled(true);
				_checkLoopPlay->setEnabled(true);
				_sliderRate->setEnabled(true);
				_lineEditRate->setEnabled(true);
				if (_checkHideFailedElement != nullptr)
					_checkHideFailedElement->setEnabled(true);
			}
			_btnExportVideo->setEnabled(true);
			_btnCustomVideo->setEnabled(true);
		}
		else
		{
			_btnFirst->setEnabled(false);
			_btnPrevious->setEnabled(false);
			_btnNext->setEnabled(false);
			_btnLast->setEnabled(false);
			_btnPlay->setEnabled(false);
			_comboRange->setEnabled(false);
			_checkLoopPlay->setEnabled(false);
			_sliderRate->setEnabled(false);
			_lineEditRate->setEnabled(false);
			_btnExportVideo->setEnabled(false);
			_btnCustomVideo->setEnabled(false);
			if (_checkHideFailedElement != nullptr)
				_checkHideFailedElement->setEnabled(false);
		}


		_comboRange->setCurrentText(guidata->_aniEveryRange);
		_checkLoopPlay->setChecked(guidata->_aniLoopPlay);
		_sliderRate->setValue(guidata->_aniFrameRate);
		_lineEditRate->setText(MxFunctions::mNumberToString(guidata->_aniFrameRate));

		_btnPlay->blockSignals(false);
		_btnFirst->blockSignals(false);
		_btnPrevious->blockSignals(false);
		_btnNext->blockSignals(false);
		_btnLast->blockSignals(false);
		_comboRange->blockSignals(false);
		_checkLoopPlay->blockSignals(false);
		_sliderRate->blockSignals(false);
		_lineEditRate->blockSignals(false);
		if (_checkHideFailedElement != nullptr)
		{
			_checkHideFailedElement->setChecked(guidata->_aniHideFailedElement);
			_checkHideFailedElement->blockSignals(false);
		}
	}

	void mPostAnimationMenuWidget::pauseAnimation()
	{
		//暂停播放
		if (_btnPlay->isChecked())
		{
			_btnPlay->clicked();
			_btnPlay->setChecked(false);
		}

	}

	void mPostAnimationMenuWidget::setHideFailedElement(bool checked)
	{
		if (_checkHideFailedElement != nullptr)
		{
			_checkHideFailedElement->blockSignals(true);
			_checkHideFailedElement->setChecked(checked);
			_checkHideFailedElement->blockSignals(false);
		}
	}
}
