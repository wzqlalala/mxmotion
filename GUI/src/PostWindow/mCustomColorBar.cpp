#include "mCustomColorBar.h"
#include "ui_mCustomColorBar.h"

#include "ColorBarWidget.h"
#include "mGlobalSignals.h"
#include "mAppConfiguration.h"
//#include "mxr/data/mPostColorTableData.h"
#include "mPostOneFrameRendData.h"
//#include "mPostCloudPanel.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mPostDataBase.h"
#include "mPostModelData.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"

using namespace PostData;

namespace PostWindow
{
	mCustomColorBar::mCustomColorBar(QWidget *parent) : MechMainWindow::mFramelessDialog(parent)
	{
		_ui = new Ui::mCustomColorBar;
		//.ui 顶层布局装进内容容器再交给基类（基类 this 的布局已被标题栏占用，
		//直接 setupUi(this) 会被 Qt 拒绝挂载）
		QWidget* content = new QWidget(this);
		_ui->setupUi(content);
		setContentWidget(content);
		//基类构造已重设窗口标志，置顶需求在此恢复
		setWindowFlag(Qt::WindowStaysOnTopHint, true);
        colorBar_ = new ColorBarWidget();

		_ui->scrollArea->setWidget(colorBar_);

		//连接信号与槽
		connect(_ui->pushButton_default, &QPushButton::clicked, this, &mCustomColorBar::slot_defaultButton);
		connect(_ui->pushButton_ok, &QPushButton::clicked, this, &mCustomColorBar::slot_okButton);
	}

	mCustomColorBar::~mCustomColorBar()
	{;
		if (_ui != nullptr)
		{
			delete _ui;
		}
	}

    void mCustomColorBar::getColorAndValue()
    {
        colorBar_->getColorAndValue();
    }

    void mCustomColorBar::getDefaultColorAndValue(QVector<QColor> colors, QVector<QString> values)
    {
        colorBar_->getDefaultColorAndValue(colors, values);
    }

    void mCustomColorBar::slot_defaultButton()
	{
        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;

		QString postName = postRend->getPostDataName();
        QPointer<mPostDataBase> postModelDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (postModelDB == nullptr)
            return;
        colorBar_->setDefaultColorBar();
        colorBarNum_ = colorBar_->getDefaultColorNum();
        int height = (colorBarNum_ + 3) * 20;
        colorBar_->setFixedSize(200, height);
        emit setDiscrete(colorBarNum_);
	}

	void mCustomColorBar::slot_okButton()
	{
        colorBar_->setEditlineHide();
		
		//设置模型颜色
		this->close();
	}

	void mCustomColorBar::mousePressEvent(QMouseEvent *event)
	{
        colorBar_->setEditlineHide();
	}

    void mCustomColorBar::getColorBarNum()
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

		QString postName = postRend->getPostDataName();
        QPointer<mPostDataBase> postModelDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (postModelDB == nullptr)
            return;

        //获取单帧结果数据
		auto* dataProxy = postRender->frameProxy(postRender->currentFrame());   // S5 门面直接替换：getCurFrameDataProxy→frameProxy(currentFrame())（F8：换帧/重装载后重取）
		if (dataProxy == nullptr)
			return;
		// 查询数据代理
		mxr::IPostRendQueryDataProxy* query_proxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(dataProxy->getRendQueryDataProxy());

        //获取颜色条阶数
        colorBarNum_ = postRender->getColorTableWindow()->GetColorCount();

        int height = (colorBarNum_ + 3) * 20;
        colorBar_->setFixedSize(200, height);
    }

}