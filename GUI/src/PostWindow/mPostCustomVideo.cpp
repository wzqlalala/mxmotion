#include "mPostCustomVideo.h"

#include <QFormLayout>
#include <QColorDialog>
#include <QDateTime>
#include <QDir>
#include <QApplication>
#include <QFileDialog>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "IViewportWidget.h"
#include "mMechGlobalSignals.h"
#include "mDockLayoutManager.h"
#include "mPostInfoHub.h"
#include "mAppConfiguration.h"

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
#include "mPostGUIData.h"

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
	mPostCustomVideoPanel::mPostCustomVideoPanel(QWidget* parent) : mPanelBase(parent)
	{
		_curPostRend = nullptr;

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
		_spinFrameStart = new QSpinBox();
		_spinFrameEnd = new QSpinBox();
		_spinFrameStart->setButtonSymbols(QAbstractSpinBox::NoButtons);
		_spinFrameStart->setValue(1);
		_spinFrameEnd->setButtonSymbols(QAbstractSpinBox::NoButtons);
		_spinFrameEnd->setValue(1);
		_spinFrameRate = new QSpinBox();
		_spinFrameRate->setMinimum(1);
		_spinFrameRate->setMaximum(99);
		_spinFrameRate->setValue(10);
		_spinFrameRate->setButtonSymbols(QAbstractSpinBox::NoButtons);
		_comboVideoType = new QComboBox();
		QStringList type;
        type << "AVI" << "GIF" << "MPEG" << "MP4";
		_comboVideoType->addItems(type);
		_btnBackground = new QPushButton();
		_btnBackground->setStyleSheet(QString("QPushButton{background-color:rgb(%1,%2,%3);}").arg(_color.red()).arg(_color.green()).arg(_color.blue()));
		_btnBackground->setFixedSize(24,24);
		
		//分辨率
		controlFold->addRow(new QLabel("分辨率"), _spinResolution);
		//帧范围
		QLabel* label = new QLabel("-");
		label->setAlignment(Qt::AlignCenter);
		label->setFixedWidth(10);
		controlFold->addRow(new QLabel("帧范围"), _spinFrameStart, label, _spinFrameEnd);
		//帧速率
		controlFold->addRow(new QLabel("帧速率"), _spinFrameRate);
		//格式
		controlFold->addRow(new QLabel("格式"), _comboVideoType);
		//背景颜色
		controlFold->addRow(new QLabel("背景颜色"), _btnBackground);
		controlFold->refreshLayout();
		m_foldWidgets.append(controlFold);
		_mainLayout->addWidget(controlFold);

		connect(_spinResolution, SIGNAL(valueChanged(int)), this, SLOT(slot_setResolution(int)));
		connect(_btnBackground, &QPushButton::clicked, this, &mPostCustomVideoPanel::slot_setBackGroundColor);


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
		connect(_spinLeftx, &QSpinBox::editingFinished, this, &mPostCustomVideoPanel::slot_setRecordCoord);
		connect(_spinLefty, &QSpinBox::editingFinished, this, &mPostCustomVideoPanel::slot_setRecordCoord);
		connect(_spinRightx, &QSpinBox::editingFinished, this, &mPostCustomVideoPanel::slot_setRecordCoord);
		connect(_spinRighty, &QSpinBox::editingFinished, this, &mPostCustomVideoPanel::slot_setRecordCoord);

		/********************保存位置********************/
		FoldSection* positionFold = new FoldSection("保存位置", this);
		_lineEditFlieName = new QLineEdit();
		_lineEditFlieName->setText("视频-1");
		_lineEditFilePath = new QLineEdit();
		QString defaultPath = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
		_lineEditFilePath->setText(defaultPath);
		QPushButton* btnPath = new QPushButton("");
		btnPath->setFixedSize(24,24);
		btnPath->setIcon(QIcon(":/mMainWidgets/Icon/homepage/dakai.png"));
		QPushButton* btnVideo = new QPushButton("导出");

		//文件名
		positionFold->addRow(new QLabel("文件名"), _lineEditFlieName);
		//路径
		positionFold->addRow(new QLabel("路径"), _lineEditFilePath, btnPath);
		//截图
		positionFold->addRow(btnVideo);
		positionFold->refreshLayout();
		m_foldWidgets.append(positionFold);
		_mainLayout->addWidget(positionFold);
		connect(btnVideo, &QPushButton::clicked, this, &mPostCustomVideoPanel::slot_customVideo);

		_mainLayout->addStretch();

		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::setAnimationOutputViewport, this, &mPostCustomVideoPanel::setCurViewName);
	}

	void mPostCustomVideoPanel::setFrameRange(int min, int max)
	{
		_spinFrameStart->setMinimum(min);
		_spinFrameStart->setMaximum(max);
		_spinFrameEnd->setMinimum(min);
		_spinFrameEnd->setMaximum(max);
	}

	void mPostCustomVideoPanel::setEndFrame(int val)
	{
		_spinFrameEnd->setValue(val);
	}

	void mPostCustomVideoPanel::slot_aniExportVideo()
	{
		QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss-zzz");
		QString path = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
		QString fileName = "视频" + time;
        //判断是否存在视频生成需要的依赖库
		QString ffmpegDir = QCoreApplication::applicationDirPath() + "/ffmpeg";
		QDir dirFFmpeg(ffmpegDir);
		if (!dirFFmpeg.exists())
		{
			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
			{
				mGlobalSignals::getInstance()->outputMessageSig(2, "缺少视频依赖文件，请在安装目录下检查ffmpeg文件夹！");
			}
			else
			{
				mGlobalSignals::getInstance()->outputMessageSig(2, "Missing video dependency files, please check the ffmpeg folder in the installation directory!");
			}
			return;
		}

		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		disconnect(postRend->getAnimationWindow(), SIGNAL(signal_EndAnimation()), this, SLOT(slot_EndAnimation()));
		connect(postRend->getAnimationWindow(), SIGNAL(signal_EndAnimation()), this, SLOT(slot_EndAnimation()));

		auto render = postRend->getPostRender();
		if (render == nullptr)
			return;

		//录制前沿用动画视口存储的点大小，保证视频中自由节点大小与设置一致
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata != nullptr && render->frameProxy(render->currentFrame()) != nullptr)   // S5 门面直接替换：getCurFrameDataProxy→frameProxy(currentFrame())（F8）
			render->setPointSize(guidata->_pointSize);

		QDir dir(path);
		if (!dir.exists())
		{
			dir.mkpath(path);
		}

		//分辨率
		int val = 1;

		auto size = postRend->size();
		auto width = size.width();
        if (width % 2 != 0)//确保值是2的倍数
        {
            width -= 1;
        }
		auto height = size.height();
        if (height % 2 != 0)//确保值是2的倍数
        {
            height -= 1;
        }
        //使用视频接口要求width, height，值为2的倍数
		postRend->getAnimationWindow()->setAnimationSize(width, height, val);

		int fRate = 10;
		postRend->getAnimationWindow()->setAnimationFrameRate(fRate);

		int fNo = mPostInfoHub::getInstance()->slot_aniStartFrame();
		int sNo = mPostInfoHub::getInstance()->slot_aniEndFrame();

		QWidget* msWindow = getCVM->getViewport(_curViewName);
		if (msWindow == nullptr)
			return;
		int x = msWindow->pos().x();
		int y = msWindow->pos().y();

		if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
		{
			mGlobalSignals::getInstance()->outputMessageSig("开始视频录制！");
		}
		else
		{
			mGlobalSignals::getInstance()->outputMessageSig("Start video recording!");
		}

		QString format = "." + _comboVideoType->currentText();
        format = format.toLower();
		postRend->getAnimationWindow()->setAnimationRect(x, y, width, height);
		// B1b(M6/V-3)：驱动点就绪检查（帧范围）
		if (!checkRecordFramesReady(postRend, fNo, sNo)) return;
		//录制×实时动画互斥（v1.1）：广播让路实时浮层（先停播放）+录制簿记置位，
		//均在就绪检查通过后的引擎调用前——录制中权威查询=本簿记（hub 聚合）
		mPostInfoHub::getInstance()->notifyRecordingStarted();
		_isStart = true;
		postRend->getAnimationWindow()->RecordAnimation(fNo, sNo, { (float)_color.red(),(float)_color.green(),(float)_color.blue() }, fileName.toStdString(), path.toStdString()/*, format.toStdString()*/);
		postRend->update();
	}

	void mPostCustomVideoPanel::updateVideoPanel()
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		_comboRange->setCurrentIndex(0);

		auto size = postRend->size();
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
		QString name = getNextVideoFilename(path);

		_lineEditFlieName->setText(name);
	}

	QString mPostCustomVideoPanel::getNextVideoFilename(const QString& dirPath, const QString& prefix, const QString& ext)
	{
		QDir dir(dirPath);
		if (!dir.exists())
			dir.mkpath(".");

        QRegularExpression re("^" + QRegularExpression::escape(prefix) + "(\\d+)\\.(\\w+)$");

		int maxNum = 0;
		QStringList files = dir.entryList(QStringList() << (prefix + "*.*"), QDir::Files);
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

	void mPostCustomVideoPanel::rePainterRecordWindow()
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

	void mPostCustomVideoPanel::slot_setBackGroundColor()
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

	void mPostCustomVideoPanel::slot_EndAnimation()
	{
		if (_isStart)
		{
			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
			{
				mGlobalSignals::getInstance()->outputMessageSig("视频录制成功！");
			}
			else
			{
				mGlobalSignals::getInstance()->outputMessageSig("Video recording successful!");
			}
			_isStart = false;
			//R-5 渲染侧销项转 GUI 自做（anim-tab-v13 D-05 升格轮·录制结束补刷）：
			//录制走查结束引擎画面可能停在走查末帧而 GUI 消费方（云图菜单/测量标注等）
			//停留在录制前帧，簿记复位后补发一次帧变化通知按当前帧收敛（与实时浮层停机
			//点补刷 mPost3DResult slot_realtimePlayToggled !playing 分支同口径）。约束：
			//录制期间 playbackState 恒 Stopped、实时浮层已让路停播，本通知经其播放态门
			//零动作不误触播放链；渲染库 record/stopRecord 为显式失败 stub 禁调用
			REBUILD_PostRend* endRend = getCVM->getPostRenderByViewName(_curViewName);
			if (endRend != nullptr)
			{
				mPostGUIData* endGui = getCVM->getGUIDataByPostRender(endRend);
				if (endGui != nullptr)
					MechCommon::mMechGlobalSignals::getInstance()->postCloudFrameChanged(endGui->_cloudFrame);
			}
			//更新截图文件名
            QString format = "." + _comboVideoType->currentText();
            format = format.toLower();
            QString name = getNextVideoFilename(_filePath, "视频-", format);
			_lineEditFlieName->setText(name);
		}
	}

	void mPostCustomVideoPanel::slot_setScreenshotPath()
	{
		QString defaultPath = qApp->applicationDirPath() + "/WorkingDir";
		QString path = QFileDialog::getExistingDirectory(this, QString("请选择文件保存路径"), defaultPath);
		if (path.trimmed() == "")
			return;
		_lineEditFilePath->setText(path);

		//更新截图文件名
		QString name = getNextVideoFilename(path);
		_lineEditFlieName->setText(name);
	}

	void mPostCustomVideoPanel::slot_customVideo()
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
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

        //判断是否存在视频生成需要的依赖库
        QString ffmpegDir = QCoreApplication::applicationDirPath() + "/ffmpeg";
        QDir dirFFmpeg(ffmpegDir);
        if (!dirFFmpeg.exists())
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, "缺少视频依赖文件，请在安装目录下检查ffmpeg文件夹！");
            }
            else
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, "Missing video dependency files, please check the ffmpeg folder in the installation directory!");
            }
            return;
        }

		disconnect(postRend->getAnimationWindow(), SIGNAL(signal_EndAnimation()), this, SLOT(slot_EndAnimation()));
		connect(postRend->getAnimationWindow(), SIGNAL(signal_EndAnimation()), this, SLOT(slot_EndAnimation()));

		auto render = postRend->getPostRender();
		if (render == nullptr)
			return;

		//录制前沿用动画视口存储的点大小，保证视频中自由节点大小与设置一致
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata != nullptr && render->frameProxy(render->currentFrame()) != nullptr)   // S5 门面直接替换：getCurFrameDataProxy→frameProxy(currentFrame())（F8）
			render->setPointSize(guidata->_pointSize);

		QDir dir(path);
		if (!dir.exists())
		{
			dir.mkpath(path);
		}
		_filePath = path;

		int val = _spinResolution->value();
		// 判定不能超过8192
		auto size = postRend->size();
		int width = size.width();
		int height = size.height();
        
        if (width % 2 != 0)//确保值是2的倍数
        {
            width -= 1;
        }
        if (height % 2 != 0)//确保值是2的倍数
        {
            height -= 1;
        }

		auto maxSize = (width > height) ? width : height;
		if (val > (8192 / maxSize))
			return;
		postRend->getAnimationWindow()->setAnimationSize(width, height, val);

		int fRate = _spinFrameRate->value();
		postRend->getAnimationWindow()->setAnimationFrameRate(fRate);

		int fNo = _spinFrameStart->value();
		int sNo = _spinFrameEnd->value();
		if (sNo < fNo)
		{
			mGlobalSignals::getInstance()->outputMessageSig("帧范围设置错误！");
			return;
		}

		QWidget* msWindow = getCVM->getViewport(_curViewName);
		if (msWindow == nullptr)
			return;
		int x = msWindow->pos().x();
		int y = msWindow->pos().y();

		if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
		{
			mGlobalSignals::getInstance()->outputMessageSig("开始视频录制！");
		}
		else
		{
			mGlobalSignals::getInstance()->outputMessageSig("Start video recording!");
		}

        QString fmt = "." + _comboVideoType->currentText();
        fmt = fmt.toLower();
		if (_comboRange->currentText() == "当前视口")
		{
			postRend->getAnimationWindow()->setAnimationRect(x, y, width, height);
			// B1b(M6/V-3)：驱动点就绪检查（帧范围）
			if (!checkRecordFramesReady(postRend, fNo, sNo)) return;
			//录制×实时动画互斥（v1.1）：广播让路实时浮层+录制簿记置位（引擎调用前）
			mPostInfoHub::getInstance()->notifyRecordingStarted();
			_isStart = true;
			postRend->getAnimationWindow()->RecordAnimation(
                fNo, sNo,
                { (float)_color.red(),(float)_color.green(),(float)_color.blue() },
                fileName.toStdString(), path.toStdString(), fmt.toStdString());
		}
		else
		{
			QPoint startPos;
			getRecordWindowInfo(startPos, width, height);
			x = startPos.x();
			y = startPos.y();
			postRend->getAnimationWindow()->setAnimationRect(x, y, width, height);
            // B1b(M6/V-3)：驱动点就绪检查（帧范围）
			if (!checkRecordFramesReady(postRend, fNo, sNo)) return;
			//录制×实时动画互斥（v1.1）：广播让路实时浮层+录制簿记置位（引擎调用前）
			mPostInfoHub::getInstance()->notifyRecordingStarted();
			_isStart = true;
			postRend->getAnimationWindow()->RecordAnimation(
                fNo, sNo,
                { (float)_color.red(),(float)_color.green(),(float)_color.blue() },
                fileName.toStdString(), path.toStdString(), fmt.toStdString());
			//截图完成关闭截图选择框
			_recordWidget->cancelBtn();
		}
		postRend->update();
	}

	void mPostCustomVideoPanel::slot_chooseRange(const QString&)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto size = postRend->size();
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

	void mPostCustomVideoPanel::getRecordWindow()
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;
		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		QWidget* msWindow = getCVM->getViewport(_curViewName);
		if (msWindow == nullptr)
			return;
		if (_recordWidget != nullptr)
		{
			delete _recordWidget;
			_recordWidget = nullptr;
		}
		QSize size(postRend->size().width(), postRend->size().height());

		//创建截图框的位置
		auto* priv = getCVM->getViewportPrivate(msWindow);
		QWidget* widget = (priv && priv->getWidget()) ? priv->getWidget()->asWidget() : nullptr;
		if (!widget) return;
		QPoint point = widget->mapToGlobal(QPoint(0, 0));
		_recordWidget = new mAnimationRecordWidget(size, point, RecordType::VideoRecord, msWindow);
		connect(_recordWidget, SIGNAL(sig_sendPos(QPoint, QPoint)), this, SLOT(slot_setPos(QPoint, QPoint)));
		connect(_recordWidget, SIGNAL(sig_saveVideo()), this, SLOT(slot_customVideo()));
		_recordWidget->getStartAndEndPos(ScreenType1::QuadScreen, this);
		_recordFlag = true;
	}

	void mPostCustomVideoPanel::getRecordWindowInfo(QPoint& startPos, int& width, int& height)
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

	void mPostCustomVideoPanel::slot_setPos(QPoint startPos, QPoint endPos)
	{
		_posF = startPos;
		_posS = endPos;

		_spinLeftx->setValue(_posF.x());
		_spinLefty->setValue(_posF.y());
		_spinRightx->setValue(_posS.x());
		_spinRighty->setValue(_posS.y());
	}

	void mPostCustomVideoPanel::slot_setRecordCoord()
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

	void mPostCustomVideoPanel::slot_setResolution(int f)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto size = postRend->size();

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

	void mPostCustomVideoPanel::setCurViewName(QString name)
	{
		_curViewName = name;
	}

	void mPostCustomVideoPanel::hideEvent(QHideEvent* event)
	{
		if (_recordWidget != nullptr)
		{
			_recordWidget->closeScreenWidget();
			_recordFlag = false;
		}
	}
}