#include "mPostCustomScreenshot.h"

#include <QFormLayout>
#include <QColorDialog>
#include <QDateTime>
#include <QDir>
#include <QApplication>
#include <QFileDialog>
#include <QGuiApplication>
#include <QScreen>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "IViewportWidget.h"
#include "mMechGlobalSignals.h"
#include "mDockLayoutManager.h"
#include "mAppConfiguration.h"
#include "mAnimationRecordWidget.h"
#include "mxr/view/CCSMRendWidget.h"
#include "mPostInfoHub.h"

//MxFunctions
#include "mGlobalSignals.h"
// S5 门面直接替换：会话头已删除（REBUILD_PostRenderSession 转私有宿主）——
// 值类型经 mxr/view/REBUILD_PostRender.h（内含 REBUILD_PostRenderTypes.h）带入


// B1b(M6/V-3)：录制驱动点帧就绪检查（R9 任务二清单项——渐进装载×录制竞态窗口
// 整改）。过渡期录制链保留外壳 getAnimationWindow（G7 record 阶段一 stub，禁迁
// 移），驱动点空判升级为就绪检查：范围内存在未就绪帧即拒绝录制（未就绪帧会截
// 到上一帧陈旧画面——R8 实证）。返回 true=就绪可录（无会话/无装载能力时放行，
// 维持 legacy 装载态现状）
static bool checkRecordFramesReady(REBUILD_PostRend* postRend, int firstFrame, int lastFrame)
{    // S5 门面直接替换：getSession()→getPostRender()（返回 shared_ptr 门面，恒在）；
    // FrameLoadState 改 mxr 作用域（REBUILD_PostRenderTypes.h）。录制链本体
    //（getAnimationWindow 保留路径）按裁定零改动
    auto session = postRend->getPostRender();
    for (int f = qMax(1, firstFrame); f <= lastFrame; ++f)
    {
        if (session->frameLoadState(f) != mxr::FrameLoadState::Ready)
        {            const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
            mGlobalSignals::getInstance()->outputMessageSig(2,
                isChinese ? QStringLiteral("帧 %1 数据未就绪（渐进装载中），请稍后重试录制").arg(f)
                          : QStringLiteral("Frame %1 not ready (progressive loading); retry recording later").arg(f),
                true);
            return false;
        }
    }
    return true;
}
//BasicData
#include "mPhysicalFieldEnum.h"

//Post
#include "mPostDataBase.h"

//窗口
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"

using namespace MxFunctions;
using namespace MechCommon;
using namespace PostData;
using namespace MechPreData;

namespace PostWindow
{
	mPostCustomScreenshotPanel::mPostCustomScreenshotPanel(QWidget* parent) : mPanelBase(parent)
	{
		_curPostRend = nullptr;
		_recordWidget = nullptr;

		_mainLayout = new QVBoxLayout(this);
		_mainLayout->setContentsMargins(2, 0, 2, 0);
		this->setStyleSheet(
			"mPostAnimationPanel { background-color: white; }"

			"QComboBox QAbstractItemView {"
			"    background-color: white;"     /* 白底   */
			"    color: black;"                /* 黑字   */
			"    selection-background-color: #3399ff;" /* 悬停高亮，可自定 */
			"    selection-color: black;"      /* 悬停时文字保持黑色 */
			"}");

		/********************控制参数********************/
		FoldSection* controlFold = new FoldSection("控制参数", this);
		_spinResolution = new QSpinBox();
		_spinResolution->setMinimum(1);
		_spinResolution->setMaximum(8);
		_spinResolution->setValue(1);
		_btnBackground = new QPushButton();
		_btnBackground->setFixedSize(24,24);
		_btnBackground->setStyleSheet(QString("QPushButton{background-color:rgb(%1,%2,%3);}").arg(_color.red()).arg(_color.green()).arg(_color.blue()));
		
		//分辨率
		controlFold->addRow(new QLabel("分辨率"), _spinResolution);
		//背景颜色
		controlFold->addRow(new QLabel("背景颜色"), _btnBackground);
		controlFold->refreshLayout();
		m_foldWidgets.append(controlFold);
		_mainLayout->addWidget(controlFold);

		connect(_spinResolution, SIGNAL(valueChanged(int)), this, SLOT(slot_setResolution(int)));
		connect(_btnBackground, &QPushButton::clicked, this, &mPostCustomScreenshotPanel::slot_setBackGroundColor);

		/********************范围********************/
		FoldSection* rangeFold = new FoldSection("范围", this);
		_comboRange = new QComboBox();
		QStringList list;
		list << "当前视口" << "选择区域";
		_comboRange->addItems(list);
		_spinLeftx = new QSpinBox();
		_spinLefty = new QSpinBox();
		_spinRightx = new QSpinBox();
		_spinRighty = new QSpinBox();
		QLabel* label1 = new QLabel(",");
		QLabel* label2 = new QLabel(",");
		_spinLeftx->setButtonSymbols(QAbstractSpinBox::NoButtons);
		_spinLefty->setButtonSymbols(QAbstractSpinBox::NoButtons);
		_spinRightx->setButtonSymbols(QAbstractSpinBox::NoButtons);
		_spinRighty->setButtonSymbols(QAbstractSpinBox::NoButtons);
		_spinLeftx->setFixedWidth(100);
		_spinLefty->setFixedWidth(100);
		_spinRightx->setFixedWidth(100);
		_spinRighty->setFixedWidth(100);

		//类型
		rangeFold->addRow(new QLabel("类型"), _comboRange);
		//左上角
		rangeFold->addRow(new QLabel("左上角"), _spinLeftx, label1, _spinLefty);
		//右下角
		rangeFold->addRow(new QLabel("右下角"), _spinRightx, label2, _spinRighty);
		rangeFold->refreshLayout();
		m_foldWidgets.append(rangeFold);
		_mainLayout->addWidget(rangeFold);
		
		connect(_comboRange, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_chooseRange(const QString&)));
		connect(_spinLeftx , &QSpinBox::editingFinished, this, &mPostCustomScreenshotPanel::slot_setRecordCoord);
		connect(_spinLefty , &QSpinBox::editingFinished, this, &mPostCustomScreenshotPanel::slot_setRecordCoord);
		connect(_spinRightx , &QSpinBox::editingFinished, this, &mPostCustomScreenshotPanel::slot_setRecordCoord);
		connect(_spinRighty , &QSpinBox::editingFinished, this, &mPostCustomScreenshotPanel::slot_setRecordCoord);


		/********************保存位置********************/
		FoldSection* positionFold = new FoldSection("保存位置", this);
		_lineEditFlieName = new QLineEdit();
		_lineEditFilePath = new QLineEdit();
		QString defaultPath = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
		_lineEditFilePath->setText(defaultPath);
		QPushButton* btnPath = new QPushButton("");
		btnPath->setFixedSize(24,24);
		btnPath->setIcon(QIcon(":/mMainWidgets/Icon/homepage/dakai.png"));
		QPushButton* btnScreenShot = new QPushButton("截图");

		//文件名
		positionFold->addRow(new QLabel("文件名"), _lineEditFlieName);
		//路径
		positionFold->addRow(new QLabel("路径"), _lineEditFilePath, btnPath);
		//截图
		positionFold->addRow(btnScreenShot);
		positionFold->refreshLayout();
		m_foldWidgets.append(positionFold);
		_mainLayout->addWidget(positionFold);
		
		connect(btnPath, &QPushButton::clicked, this, &mPostCustomScreenshotPanel::slot_setScreenshotPath);
		connect(btnScreenShot, &QPushButton::clicked, this, &mPostCustomScreenshotPanel::slot_customScreenshot);

		_mainLayout->addStretch();

		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::setAnimationOutputViewport, this, &mPostCustomScreenshotPanel::setCurViewName);
	}

	void mPostCustomScreenshotPanel::slot_aniQuickScreenshot()
	{
		QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss-zzz");
		QString path = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
		QString fileName = "截图" + time;

		if (getCVM->getPostRenderWidget())
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			if (postRend == nullptr) return;

			disconnect(postRend->getAnimationWindow(), SIGNAL(signal_EndAnimation()), this, SLOT(slot_EndAnimation()));
			connect(postRend->getAnimationWindow(), SIGNAL(signal_EndAnimation()), this, SLOT(slot_EndAnimation()));

			auto render = postRend->getPostRender();
			if (render == nullptr)
				return;

			//分辨率
			int val = 1;

			auto size = postRend->size();
			auto width = size.width();
			auto height = size.height();
			postRend->getAnimationWindow()->setAnimationSize(width, height, val);

			//修改动画模块截图
			////暂停动画
			//MechCommon::mMechGlobalSignals::getInstance()->pauseAnimation();
			////获取当前帧
			//int fNo = mInfoWindow::getInstance()->slot_aniCurFrame();

			//根据动画状态获取对应帧（R25 修复，审计遗漏-M1）：原按视口名
			//view.contains("Animation") 分流帧选——第二视口链拆除后视口名恒为云图名，
			//动画分支恒死（时间控自定义动画播放期 _cloudFrame 不跟随，单帧截图会取到
			//生成前静态帧）。单视口下截图目标恒为当前云图视口：已生成动画
			//（_generateAnimate）直读渲染会话当前帧（绝对帧号，帧控/时间控播放中均为
			//实际显示帧，静态云图切帧亦经 seekFrame 同域）；静态云图态仍取 _cloudFrame。
			//录制链 API（getAnimationWindow/RecordScreen/就绪检查）保持不动
			int frameID;
			mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
			if (guidata->_generateAnimate)
			{
				frameID = render->currentFrame();
			}
			else
			{
				frameID = guidata->_cloudFrame;
			}

			QWidget* msWindow = getCVM->getSubWindowFromRend(postRend);
			if (msWindow == nullptr)
				return;
			int x = msWindow->pos().x();
			int y = msWindow->pos().y();

			QDir dir(path);
			if (!dir.exists())
			{
				dir.mkpath(path);
			}
			_filePath = path;
			_fileName = fileName;

			postRend->getAnimationWindow()->setAnimationRect(x, y, width, height);
			// B1b(M6/V-3)：驱动点就绪检查（单帧）
			if (!checkRecordFramesReady(postRend, frameID, frameID)) return;
			//录制×实时动画互斥（v1.1）：广播让路实时浮层+录制簿记置位（引擎调用前）；
			//置位自检查通过后的成功路径起（原入口早期置位在就绪检查失败早退时残留
			//stale true，会令互斥查询永久误报录制中）
			mPostInfoHub::getInstance()->notifyRecordingStarted();
			_isStart = true;
			postRend->getAnimationWindow()->RecordScreen(frameID, { (float)_color.red(),(float)_color.green(),(float)_color.blue() }, fileName.toStdString(), path.toStdString());
			postRend->update();
		}
		if (getCVM->getPreRenderWidget())
		{
			CCSMRendWidget* preRend = getCVM->getPreRenderWidget();
			if (preRend == nullptr) return;

			disconnect(preRend, SIGNAL(signal_CaptureFinished()), this, SLOT(slot_EndAnimation()));
			connect(preRend, SIGNAL(signal_CaptureFinished()), this, SLOT(slot_EndAnimation()));

			_isStart = true;

			//分辨率
			float val = 1.0;
			auto size = preRend->size();
			auto width = size.width();
			auto height = size.height();

			QWidget* msWindow = getCVM->getSubWindowFromRend(preRend);
			if (msWindow == nullptr)
				return;
			int x = msWindow->pos().x();
			int y = msWindow->pos().y();

			QDir dir(path);
			if (!dir.exists())
			{
				dir.mkpath(path);
			}
			_filePath = path;
			_fileName = fileName;

			QString pathName = path + "/" + fileName + ".png";

			preRend->FastCapture(pathName, { (float)x,(float)y }, { (float)width, (float)height }, { (float)_color.red(),(float)_color.green(),(float)_color.blue() }, val);
		}
	}

    void mPostCustomScreenshotPanel::aniQuickScreenshot(QString _path)
    {
        QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss-zzz");
        QString path = _path;
        qDebug() << "path is:" << path;
        QString fileName = "截图" + time;

        if (getCVM->getPostRenderWidget())
        {
            REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
            if (postRend == nullptr) return;

            disconnect(postRend->getAnimationWindow(), SIGNAL(signal_EndAnimation()), this, SLOT(slot_EndAnimation()));
            connect(postRend->getAnimationWindow(), SIGNAL(signal_EndAnimation()), this, SLOT(slot_EndAnimation()));

            auto render = postRend->getPostRender();
            if (render == nullptr)
                return;

            //分辨率
            int val = 1;

            auto size = postRend->size();  
            auto width = size.width();
            auto height = size.height();
            postRend->getAnimationWindow()->setAnimationSize(width, height, val);

            //根据动画状态获取对应帧（R25 修复，审计遗漏-M1）：原按视口名
            //view.contains("Animation") 分流帧选——第二视口链拆除后视口名恒为云图名，
            //动画分支恒死（时间控自定义动画播放期 _cloudFrame 不跟随，单帧截图会取到
            //生成前静态帧）。单视口下截图目标恒为当前云图视口：已生成动画
            //（_generateAnimate）直读渲染会话当前帧（绝对帧号，帧控/时间控播放中均为
            //实际显示帧，静态云图切帧亦经 seekFrame 同域）；静态云图态仍取 _cloudFrame。
            //录制链 API（getAnimationWindow/RecordScreen/就绪检查）保持不动
            int frameID;
            mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
            if (guidata->_generateAnimate)
            {
                frameID = render->currentFrame();
            }
            else
            {
                frameID = guidata->_cloudFrame;
            }

            QWidget* msWindow = getCVM->getSubWindowFromRend(postRend);
            if (msWindow == nullptr)
                return;
            int x = msWindow->pos().x();
            int y = msWindow->pos().y();

            QDir dir(path);
            if (!dir.exists())
            {
                dir.mkpath(path);
            }
            _filePath = path;
            _fileName = fileName;

            postRend->getAnimationWindow()->setAnimationRect(x, y, width, height);
            // B1b(M6/V-3)：驱动点就绪检查（单帧）
			if (!checkRecordFramesReady(postRend, frameID, frameID)) return;
			//录制×实时动画互斥（v1.1）：广播让路实时浮层+录制簿记置位（引擎调用前）
			mPostInfoHub::getInstance()->notifyRecordingStarted();
			_isStart = true;
			postRend->getAnimationWindow()->RecordScreen(frameID, { (float)_color.red(),(float)_color.green(),(float)_color.blue() }, fileName.toStdString(), path.toStdString());
            postRend->update();
        }

        if (getCVM->getPreRenderWidget())
        {
            CCSMRendWidget* preRend = getCVM->getPreRenderWidget();
            if (preRend == nullptr) return;

            disconnect(preRend, SIGNAL(signal_CaptureFinished()), this, SLOT(slot_EndAnimation()));
            connect(preRend, SIGNAL(signal_CaptureFinished()), this, SLOT(slot_EndAnimation()));

            _isStart = true;

            //分辨率
            float val = 1.0;
            auto size = preRend->size();
            auto width = size.width();
            auto height = size.height();

            QWidget* msWindow = getCVM->getSubWindowFromRend(preRend);
            if (msWindow == nullptr)
                return;
            int x = msWindow->pos().x();
            int y = msWindow->pos().y();

            QDir dir(path);
            if (!dir.exists())
            {
                dir.mkpath(path);
            }
            _filePath = path;
            _fileName = fileName;

            QString pathName = path + "/" + fileName + ".png";

            preRend->FastCapture(pathName, { (float)x,(float)y }, { (float)width, (float)height }, { (float)_color.red(),(float)_color.green(),(float)_color.blue() }, val);
        }
    }

	void mPostCustomScreenshotPanel::updateScreenshotPanel()
	{
		mBaseRend* rend = nullptr;
		auto viewportPrivate = VMManager->getCurrentViewportPrivate();
		if (viewportPrivate == nullptr)
		{
			return;
		}
		rend = viewportPrivate->getRendWidget();
		if (rend == nullptr)
		{
			return;
		}
		_comboRange->setCurrentIndex(0);

		auto size = rend->size();
		auto width = size.width();
		auto height = size.height();

		_spinLeftx->setRange(0, width);
		_spinLefty->setRange(0, height);
		_spinRightx->setRange(0, width);
		_spinRighty->setRange(0, height);
		_spinLeftx->setValue(0);
		_spinLefty->setValue(0);
		_spinRightx->setValue(width);
		_spinRighty->setValue(height);
		_spinLeftx->setEnabled(false);
		_spinLefty->setEnabled(false);
		_spinRightx->setEnabled(false);
		_spinRighty->setEnabled(false);

		QString path = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
		_lineEditFilePath->setText(path);
		//更新截图文件名
		QString name = getNextScreenshotFilename(path);

		_lineEditFlieName->setText(name);
	}

	QString mPostCustomScreenshotPanel::getNextScreenshotFilename(const QString& dirPath, const QString& prefix, const QString& ext)
	{
		QDir dir(dirPath);
		if (!dir.exists())
			dir.mkpath(".");

		QRegularExpression re("^" + QRegularExpression::escape(prefix) + "(\\d+)\\." + QRegularExpression::escape(ext) + "$");

		int maxNum = 0;
		QStringList files = dir.entryList(QStringList() << (prefix + "*." + ext), QDir::Files);
		for (const QString& file : files) 
		{
			QRegularExpressionMatch match = re.match(file);
			if (match.hasMatch()) {
				int num = match.captured(1).toInt();
				if (num > maxNum) maxNum = num;
			}
		}
		return QString("%1%2").arg(prefix).arg(maxNum + 1);
	}

	void mPostCustomScreenshotPanel::slot_setResolution(int f)
	{
		mBaseRend* rend = nullptr;
		auto viewportPrivate = VMManager->getCurrentViewportPrivate();
		if (viewportPrivate == nullptr)
		{
			return;
		}
		rend = viewportPrivate->getRendWidget();
		if (rend == nullptr)
		{
			return;
		}

		auto size = rend->size();

		auto width = size.width();
		auto height = size.height();
		auto maxSize = (width > height) ? width : height;
		int maxValue = std::floor(8192 / maxSize);
		_spinResolution->setMaximum(maxValue);
		if (f > maxValue)
		{
			_spinResolution->setValue(maxValue);
		}
	}

	void mPostCustomScreenshotPanel::slot_setBackGroundColor()
	{
		QPushButton* senderButton = qobject_cast<QPushButton*>(sender());

		QString str;
		if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
		{
			str = "选择颜色";
		}
		else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
		{
			str = "Choose Color";
		}
		// 显示颜色选择对话框，并获取用户选择的颜色
		QColor color = QColorDialog::getColor(Qt::white, this, str);

		if (color.isValid()) {
			senderButton->setStyleSheet(QString("QPushButton{background-color:rgb(%1,%2,%3);}").arg(color.red()).arg(color.green()).arg(color.blue()));
			_color = color;
		}
	}

	void mPostCustomScreenshotPanel::slot_setScreenshotPath()
	{
		QString defaultPath = qApp->applicationDirPath() + "/WorkingDir";
		QString path = QFileDialog::getExistingDirectory(this, QString("请选择文件保存路径"), defaultPath);
		if (path.trimmed() == "")
			return;
		_lineEditFilePath->setText(path);

		//更新截图文件名
		QString name = getNextScreenshotFilename(path);
		_lineEditFlieName->setText(name);
	}

	void mPostCustomScreenshotPanel::slot_EndAnimation()
	{
		if (_isStart)
		{
			mGlobalSignals::getInstance()->outputMessageSig(QString("截图成功(%1/%2)").arg(_filePath).arg(_fileName));
			_isStart = false;
			//R-5 渲染侧销项转 GUI 自做（anim-tab-v13 D-05 升格轮·录制结束补刷）：
			//录制/抓屏回执后补发一次帧变化通知按当前帧收敛（与实时浮层停机点补刷同
			//口径；录制期间 playbackState 恒 Stopped、实时浮层已让路停播，本通知经其
			//播放态门零动作不误触播放链）。前处理抓屏上下文无云图视口→判空静默跳过
			REBUILD_PostRend* endRend = getCVM->getPostRenderWidget();
			if (endRend != nullptr)
			{
				mPostGUIData* endGui = getCVM->getGUIDataByPostRender(endRend);
				if (endGui != nullptr)
					MechCommon::mMechGlobalSignals::getInstance()->postCloudFrameChanged(endGui->_cloudFrame);
			}
			//更新截图文件名
			QString name = getNextScreenshotFilename(_filePath);
			_lineEditFlieName->setText(name);
		}
	}

	void mPostCustomScreenshotPanel::slot_chooseRange(const QString&)
	{
		mBaseRend* rend = nullptr;
		auto viewportPrivate = VMManager->getCurrentViewportPrivate();
		if (viewportPrivate == nullptr)
		{
			return;
		}
		rend = viewportPrivate->getRendWidget();
		if (rend == nullptr)
		{
			return;
		}

		auto size = rend->size();
		auto width = size.width();
		auto height = size.height();

		if (_comboRange->currentText() == "当前视口")
		{
			_btnBackground->setEnabled(true);
			_spinLeftx->setEnabled(false);
			_spinLefty->setEnabled(false);
			_spinRightx->setEnabled(false);
			_spinRighty->setEnabled(false);
			_spinLeftx->setValue(0);
			_spinLefty->setValue(0);
			_spinRightx->setValue(width);
			_spinRighty->setValue(height);

			_recordWidget->closeScreenWidget();
		}
		else if (_comboRange->currentText() == "选择区域")
		{
			_btnBackground->setEnabled(false);
			_spinLeftx->setEnabled(true);
			_spinLefty->setEnabled(true);
			_spinRightx->setEnabled(true);
			_spinRighty->setEnabled(true);
			getRecordWindow();
		}
	}

	void mPostCustomScreenshotPanel::getRecordWindow()
	{
		mBaseRend* rend = nullptr;
		auto viewportPrivate = VMManager->getCurrentViewportPrivate();
		if (viewportPrivate == nullptr)
		{
			return;
		}
		rend = viewportPrivate->getRendWidget();
		if (rend == nullptr)
		{
			return;
		}

		QWidget* msWindow = getCVM->getSubWindowFromRend(rend);
		if (msWindow == nullptr)
			return;
		if (_recordWidget != nullptr)
		{
			delete _recordWidget;
			_recordWidget = nullptr;
		}
		QSize size(rend->size().width(), rend->size().height());

		//创建截图框的位置
		auto* priv = getCVM->getViewportPrivate(msWindow);
		QWidget* widget = (priv && priv->getWidget()) ? priv->getWidget()->asWidget() : nullptr;
		if (!widget) return;
		QPoint point = widget->mapToGlobal(QPoint(0, 0));
		_recordWidget = new mAnimationRecordWidget(size, point, RecordType::ShotRecord, this);
		connect(_recordWidget, SIGNAL(sig_sendPos(QPoint, QPoint)), this, SLOT(slot_setPos(QPoint, QPoint)));
		connect(_recordWidget, SIGNAL(sig_saveShot()), this, SLOT(slot_customScreenshot()));
		_recordWidget->getStartAndEndPos(ScreenType1::QuadScreen, this);
		_recordFlag = true;
	}

	void mPostCustomScreenshotPanel::getRecordWindowInfo(QPoint& startPos, int& width, int& height)
	{
		width = abs(_posF.x() - _posS.x());
		height = abs(_posF.y() - _posS.y());

		if (_posF.x() < _posS.x())
		{
			if (_posF.y() > _posS.y())
			{
				startPos = QPoint(_posF.x(), _posS.y());
			}
			else
			{
				startPos = QPoint(_posF.x(), _posF.y());
			}
		}
		else
		{
			if (_posF.y() > _posS.y())
			{
				startPos = QPoint(_posS.x(), _posS.y());
			}
			else
			{
				startPos = QPoint(_posS.x(), _posF.y());
			}
		}
	}

	void mPostCustomScreenshotPanel::slot_customScreenshot()
	{
		QString path = _lineEditFilePath->text().trimmed();
		QString fileName = _lineEditFlieName->text();
		if (path.isEmpty() || fileName.isEmpty())
		{
			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
			{
				mGlobalSignals::getInstance()->outputMessageSig("存储路径或文件名为空！");
			}
			else
			{
				mGlobalSignals::getInstance()->outputMessageSig("The storage path or file name is empty!");
			}
			return;
		}

		if (getCVM->getPostRenderWidget())
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			if (postRend == nullptr) return;

			disconnect(postRend->getAnimationWindow(), SIGNAL(signal_EndAnimation()), this, SLOT(slot_EndAnimation()));
			connect(postRend->getAnimationWindow(), SIGNAL(signal_EndAnimation()), this, SLOT(slot_EndAnimation()));

			auto render = postRend->getPostRender();
			if (render == nullptr)
				return;
			int val = _spinResolution->value();
			// 判定不能超过8192
			auto size = postRend->size();
			int width = size.width();
			int height = size.height();
			auto maxSize = (width > height) ? width : height;
			if (val > (8192 / maxSize))
				return;

			//暂停动画
			//MechCommon::mMechGlobalSignals::getInstance()->pauseAnimation();
			//postRend->getAnimationWindow()->setAnimationSize(width, height, val);

			//根据动画状态获取对应帧（R25 修复，审计遗漏-M1）：原按视口名
			//view.contains("Animation") 分流帧选——第二视口链拆除后视口名恒为云图名，
			//动画分支恒死（时间控自定义动画播放期 _cloudFrame 不跟随，单帧截图会取到
			//生成前静态帧）。单视口下截图目标恒为当前云图视口：已生成动画
			//（_generateAnimate）直读渲染会话当前帧（绝对帧号，帧控/时间控播放中均为
			//实际显示帧，静态云图切帧亦经 seekFrame 同域）；静态云图态仍取 _cloudFrame。
			//录制链 API（getAnimationWindow/RecordScreen/就绪检查）保持不动
			int frameID;
			mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
			if (guidata->_generateAnimate)
			{
				frameID = render->currentFrame();
			}
			else
			{
				frameID = guidata->_cloudFrame;
			}

			QWidget* msWindow = getCVM->getSubWindowFromRend(postRend);
			if (msWindow == nullptr)
				return;
			int x = msWindow->pos().x();
			int y = msWindow->pos().y();

			QDir dir(path);
			if (!dir.exists())
			{
				dir.mkpath(path);
			}
			_filePath = path;
			_fileName = fileName;

			if (_comboRange->currentText() == "当前视口")
			{
				postRend->getAnimationWindow()->setAnimationRect(x, y, width, height);
				// B1b(M6/V-3)：驱动点就绪检查（单帧）
			if (!checkRecordFramesReady(postRend, frameID, frameID)) return;
			//录制×实时动画互斥（v1.1）：广播让路实时浮层+录制簿记置位（引擎调用前）
			mPostInfoHub::getInstance()->notifyRecordingStarted();
			_isStart = true;
			postRend->getAnimationWindow()->RecordScreen(frameID, { (float)_color.red(),(float)_color.green(),(float)_color.blue() }, fileName.toStdString(), path.toStdString());
			}
			else
			{
				QPoint startPos;
				getRecordWindowInfo(startPos, width, height);
				x = startPos.x();
				y = startPos.y();
				postRend->getAnimationWindow()->setAnimationRect(x, y, width, height);
				// B1b(M6/V-3)：驱动点就绪检查（单帧）
			if (!checkRecordFramesReady(postRend, frameID, frameID)) return;
			//录制×实时动画互斥（v1.1）：广播让路实时浮层+录制簿记置位（引擎调用前）
			mPostInfoHub::getInstance()->notifyRecordingStarted();
			_isStart = true;
			postRend->getAnimationWindow()->RecordScreen(frameID, { (float)_color.red(),(float)_color.green(),(float)_color.blue() }, fileName.toStdString(), path.toStdString());

				//截图完成关闭截图选择框
				_recordWidget->cancelBtn();
			}
			postRend->update();
		}
		if (getCVM->getPreRenderWidget())
		{
			CCSMRendWidget* preRend = getCVM->getPreRenderWidget();
			if (preRend == nullptr) return;

			disconnect(preRend, SIGNAL(signal_CaptureFinished()), this, SLOT(slot_EndAnimation()));
			connect(preRend, SIGNAL(signal_CaptureFinished()), this, SLOT(slot_EndAnimation()));

			_isStart = true;

			//分辨率
			float val = _spinResolution->value();
			auto size = preRend->size();
			auto width = size.width();
			auto height = size.height();

			QWidget* msWindow = getCVM->getSubWindowFromRend(preRend);
			if (msWindow == nullptr)
				return;
			int x = msWindow->pos().x();
			int y = msWindow->pos().y();

			QDir dir(path);
			if (!dir.exists())
			{
				dir.mkpath(path);
			}
			_filePath = path;
			_fileName = fileName;

			QString pathName = path + "/" + fileName + ".png";

			if (_comboRange->currentText() == "当前视口")
			{
				preRend->FastCapture(pathName, { (float)x,(float)y }, { (float)width, (float)height }, { (float)_color.red(),(float)_color.green(),(float)_color.blue() }, val);
			}
			else
			{
				QPoint startPos;
				getRecordWindowInfo(startPos, width, height);
				x = startPos.x();
				y = startPos.y();
				preRend->FastCapture(pathName, { (float)x,(float)y }, { (float)width, (float)height }, { (float)_color.red(),(float)_color.green(),(float)_color.blue() }, val);

				//截图完成关闭截图选择框
				_recordWidget->cancelBtn();
			}
		}
	}

	void mPostCustomScreenshotPanel::slot_setPos(QPoint startPos, QPoint endPos)
	{
		_posF = startPos;
		_posS = endPos;

		_spinLeftx->setValue(_posF.x());
		_spinLefty->setValue(_posF.y());
		_spinRightx->setValue(_posS.x());
		_spinRighty->setValue(_posS.y());
	}

	void mPostCustomScreenshotPanel::slot_setRecordCoord()
	{
		int xLeft = _spinLeftx->value();
		int yLeft = _spinLefty->value();
		int xRight = _spinRightx->value();
		int yRight = _spinRighty->value();

		QPoint start(xLeft, yLeft);
		QPoint end(xRight, yRight);
		_recordWidget->setStart(start);
		_recordWidget->setEnd(end);
	}

	void mPostCustomScreenshotPanel::rePainterRecordWindow()
	{
		if (_recordFlag)
		{
			if (_recordWidget != nullptr)
			{
				_recordWidget->closeScreenWidget();
				getRecordWindow();
			}
		}
	}

	void mPostCustomScreenshotPanel::setCurViewName(QString name)
	{
		_curViewName = name;
	}

	void mPostCustomScreenshotPanel::hideEvent(QHideEvent* event)
	{
		if (_recordWidget != nullptr)
		{
			_recordWidget->closeScreenWidget();
			_recordFlag = false;
		}
	}
}