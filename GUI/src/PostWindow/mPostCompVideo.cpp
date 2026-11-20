#include "mPostCompVideo.h"
#include "mCommonFunctions.h"
#include "mGlobalSignals.h"
#include "mAppConfiguration.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mxr/view/REBUILD_PostModelRender.h"
#include "mVideoWindow.h"
#include "mPostInfoHub.h"
#include "mMechGlobalSignals.h"
//dock 已取消（2026-11-17）：视频显示形态=叠加/背景/独立视口，不再依赖停靠布局

#include <QRegExp>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegExpValidator>
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <functional>

namespace PostWindow
{
    mPostCompVideoPanel::mPostCompVideoPanel(QWidget *parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);

        /********************视频设置********************/
        _circularFold = new FoldSection("视频设置", this);

        _spinBox_ratenum = new QSpinBox(this);
        _spinBox_ratenum->setObjectName(QString::fromUtf8("spinBox_ratenum"));
        _spinBox_ratenum->setMinimum(1);
        _spinBox_ratenum->setMaximum(1000);
        _spinBox_ratenum->setValue(30);
        _circularFold->addRow(new PanelLabel("帧速率: ", this), _spinBox_ratenum, new PanelLabel("fps", this));
        _spinBox_ratenum->show();

        _checkBox_loopPlay = new QCheckBox(this);
        _checkBox_loopPlay->setObjectName(QString::fromUtf8("checkBox_loopPlay"));
        _checkBox_loopPlay->setText(QString::fromUtf8(""));
        _circularFold->addRow(new PanelLabel("循环播放: ", this), _checkBox_loopPlay);
        _checkBox_loopPlay->show();

        _circularFold->refreshLayout();
        _mainLayout->addWidget(_circularFold);

        /********************动画同步********************/
        _syncFold = new FoldSection("动画同步", this);

        _checkBox_syncAnim = new QCheckBox(this);
        _checkBox_syncAnim->setObjectName(QString::fromUtf8("checkBox_syncAnim"));
        _checkBox_syncAnim->setText(QString::fromUtf8(""));
        _syncFold->addRow(new PanelLabel("与动画同步: ", this), _checkBox_syncAnim);
        _checkBox_syncAnim->show();

        _comboBox_syncMapping = new QComboBox(this);
        _comboBox_syncMapping->setObjectName(QString::fromUtf8("comboBox_syncMapping"));
        _comboBox_syncMapping->setEnabled(false);
        _comboBox_syncMapping->addItem(QString::fromUtf8("按时间对齐（秒对秒）"));
        _comboBox_syncMapping->addItem(QString::fromUtf8("全程映射（起止对齐）"));
        _syncFold->addRow(new PanelLabel("时间映射: ", this), _comboBox_syncMapping);
        _comboBox_syncMapping->show();

        _doubleSpinBox_syncOffset = new QDoubleSpinBox(this);
        _doubleSpinBox_syncOffset->setObjectName(QString::fromUtf8("doubleSpinBox_syncOffset"));
        _doubleSpinBox_syncOffset->setEnabled(false);
        _doubleSpinBox_syncOffset->setMinimum(-3600.0);
        _doubleSpinBox_syncOffset->setMaximum(3600.0);
        _doubleSpinBox_syncOffset->setDecimals(3);
        _doubleSpinBox_syncOffset->setSingleStep(0.1);
        _doubleSpinBox_syncOffset->setValue(0.0);
        _doubleSpinBox_syncOffset->setSuffix(QString::fromUtf8(" s"));
        _syncFold->addRow(new PanelLabel("时间偏移: ", this), _doubleSpinBox_syncOffset);
        _doubleSpinBox_syncOffset->show();

        _syncFold->refreshLayout();
        _mainLayout->addWidget(_syncFold);

        /********************叠加显示********************/
        _overlayFold = new FoldSection("叠加显示", this);

        _checkBox_overlay = new QCheckBox(this);
        _checkBox_overlay->setObjectName(QString::fromUtf8("checkBox_overlay"));
        _checkBox_overlay->setText(QString::fromUtf8(""));
        _overlayFold->addRow(new PanelLabel("叠加到视口: ", this), _checkBox_overlay);
        _checkBox_overlay->show();

        _checkBox_overlayBg = new QCheckBox(this);
        _checkBox_overlayBg->setObjectName(QString::fromUtf8("checkBox_overlayBg"));
        _checkBox_overlayBg->setText(QString::fromUtf8(""));
        _checkBox_overlayBg->setEnabled(false);
        _overlayFold->addRow(new PanelLabel("作为背景: ", this), _checkBox_overlayBg);
        _checkBox_overlayBg->show();

        _checkBox_viewportDisplay = new QCheckBox(this);
        _checkBox_viewportDisplay->setObjectName(QString::fromUtf8("checkBox_viewportDisplay"));
        _checkBox_viewportDisplay->setText(QString::fromUtf8(""));
        _overlayFold->addRow(new PanelLabel("独立视口: ", this), _checkBox_viewportDisplay);
        _checkBox_viewportDisplay->show();

        _checkBox_fillViewport = new QCheckBox(this);
        _checkBox_fillViewport->setObjectName(QString::fromUtf8("checkBox_fillViewport"));
        _checkBox_fillViewport->setText(QString::fromUtf8(""));
        _checkBox_fillViewport->setEnabled(false);
        _checkBox_fillViewport->setChecked(true);   //默认铺满（用户裁定：铺满为用户可选项）
        _overlayFold->addRow(new PanelLabel("铺满视口: ", this), _checkBox_fillViewport);
        _checkBox_fillViewport->show();

        _comboBox_overlayPos = new QComboBox(this);
        _comboBox_overlayPos->setObjectName(QString::fromUtf8("comboBox_overlayPos"));
        _comboBox_overlayPos->setEnabled(false);
        _comboBox_overlayPos->addItem(QString::fromUtf8("左上"));
        _comboBox_overlayPos->addItem(QString::fromUtf8("上中"));
        _comboBox_overlayPos->addItem(QString::fromUtf8("右上"));
        _comboBox_overlayPos->addItem(QString::fromUtf8("左中"));
        _comboBox_overlayPos->addItem(QString::fromUtf8("居中"));
        _comboBox_overlayPos->addItem(QString::fromUtf8("右中"));
        _comboBox_overlayPos->addItem(QString::fromUtf8("左下"));
        _comboBox_overlayPos->addItem(QString::fromUtf8("下中"));
        _comboBox_overlayPos->addItem(QString::fromUtf8("右下"));
        _comboBox_overlayPos->setCurrentIndex(4);
        _overlayFold->addRow(new PanelLabel("显示位置: ", this), _comboBox_overlayPos);
        _comboBox_overlayPos->show();

        _spinBox_overlayScale = new QSpinBox(this);
        _spinBox_overlayScale->setObjectName(QString::fromUtf8("spinBox_overlayScale"));
        _spinBox_overlayScale->setEnabled(false);
        _spinBox_overlayScale->setMinimum(10);
        _spinBox_overlayScale->setMaximum(100);
        _spinBox_overlayScale->setValue(40);
        _spinBox_overlayScale->setSuffix(QString::fromUtf8(" %"));
        _overlayFold->addRow(new PanelLabel("画面缩放: ", this), _spinBox_overlayScale);
        _spinBox_overlayScale->show();

        _spinBox_overlayOpacity = new QSpinBox(this);
        _spinBox_overlayOpacity->setObjectName(QString::fromUtf8("spinBox_overlayOpacity"));
        _spinBox_overlayOpacity->setEnabled(false);
        _spinBox_overlayOpacity->setMinimum(10);
        _spinBox_overlayOpacity->setMaximum(100);
        _spinBox_overlayOpacity->setValue(80);
        _spinBox_overlayOpacity->setSuffix(QString::fromUtf8(" %"));
        _overlayFold->addRow(new PanelLabel("不透明度: ", this), _spinBox_overlayOpacity);
        _spinBox_overlayOpacity->show();

        _overlayFold->refreshLayout();
        _mainLayout->addWidget(_overlayFold);

        /********************视频导入********************/
        _importFold = new FoldSection("视频导入", this);

        _lineEdit_videoPath = new QLineEdit(this);
        _lineEdit_videoPath->setObjectName(QString::fromUtf8("lineEdit_videoPath"));
        _lineEdit_videoPath->setEnabled(false);
        _importFold->addRow(new PanelLabel("视频路径: ", this), _lineEdit_videoPath);
        _lineEdit_videoPath->show();

        _pushButton_importVideo = new QPushButton(this);
        _pushButton_importVideo->setObjectName(QString::fromUtf8("pushButton_importVideo"));
        _pushButton_importVideo->setMinimumSize(QSize(80, 0));

        _pushButton_playVideo = new QPushButton(this);
        _pushButton_playVideo->setObjectName(QString::fromUtf8("pushButton_playVideo"));
        _pushButton_playVideo->setMinimumSize(QSize(80, 0));
        _pushButton_playVideo->setEnabled(false);
        _importFold->addRow(_pushButton_importVideo, _pushButton_playVideo, QString());
        _pushButton_importVideo->show();
        _pushButton_playVideo->show();

        _importFold->refreshLayout();
        _mainLayout->addWidget(_importFold);

        /********************结束********************/
        _mainLayout->addStretch();

        initialUI();

        // 导入/播放按钮（原 setupUi 的 connectSlotsByName 自动连接改为显式连接）
        connect(_pushButton_importVideo, &QPushButton::clicked, this, &mPostCompVideoPanel::slot_importVideo);
        connect(_pushButton_playVideo, &QPushButton::clicked, this, &mPostCompVideoPanel::slot_playVideo);

        // 切换界面语言
        connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)), this, SLOT(slot_changeLanguage(int)));
        slot_changeLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);

        connect(_checkBox_loopPlay, &QCheckBox::toggled, this, [=](bool checked) {
            mVideoWindow::getInstance()->setLoopPlay(checked);
        });
        //帧速率即时生效：数值变化即下发（播放中实时变速——解码线程逐帧快照
        //强制帧率；dock 控制条/叠加手动播放同享此值，不再依赖播放按钮重读）
        connect(_spinBox_ratenum, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int fps) {
                if (mVideoWindow* videoInstance = mVideoWindow::getInstance())
                    videoInstance->setPlayFps(fps);
            });
        //初始态补推：initialUI 的 setChecked(true) 先于本 connect，toggled 未被
        //捕获——不补推则解码线程默认不循环（原靠播放按钮点击时兜底，现收口于此）
        mVideoWindow::getInstance()->setLoopPlay(_checkBox_loopPlay->isChecked());

        //视频原始帧率：改连 mVideoWindow 信号（替代原 AddFpsReadyFunc 裸 this 回调——
        //单例持 bind(this) 的 std::function，面板销毁后回调悬空且重复注册）
        connect(mVideoWindow::getInstance(), &mVideoWindow::sigVideoFpsReady,
            this, &mPostCompVideoPanel::OnVideoFpsReady);

        //播放按钮文案随播放态切换（播放视频/暂停视频）
        connect(mVideoWindow::getInstance(), &mVideoWindow::sigPlayStateChanged,
            this, &mPostCompVideoPanel::slot_playStateChanged);

        //动画同步参数 → mVideoWindow
        connect(_checkBox_syncAnim, &QCheckBox::toggled, this, &mPostCompVideoPanel::slot_syncToggled);
        connect(_comboBox_syncMapping, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mPostCompVideoPanel::slot_syncMappingChanged);
        connect(_doubleSpinBox_syncOffset, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &mPostCompVideoPanel::slot_syncOffsetChanged);

        //视口叠加参数 → mVideoWindow；叠加件被动失效（视口关闭）经信号回弹勾选
        connect(_checkBox_overlay, &QCheckBox::toggled, this, &mPostCompVideoPanel::slot_overlayToggled);
        connect(_checkBox_overlayBg, &QCheckBox::toggled, this, &mPostCompVideoPanel::slot_overlayBgToggled);
        connect(_comboBox_overlayPos, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mPostCompVideoPanel::slot_overlayPosChanged);
        connect(_spinBox_overlayScale, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &mPostCompVideoPanel::slot_overlayScaleChanged);
        connect(_spinBox_overlayOpacity, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &mPostCompVideoPanel::slot_overlayOpacityChanged);
        connect(mVideoWindow::getInstance(), &mVideoWindow::sigOverlayActiveChanged,
            this, &mPostCompVideoPanel::slot_overlayActiveChanged);

        //独立视口（与叠加互斥；互斥另一方向的勾选回弹经 sigViewportDisplayChanged）
        connect(_checkBox_viewportDisplay, &QCheckBox::toggled,
            this, &mPostCompVideoPanel::slot_viewportDisplayToggled);
        connect(mVideoWindow::getInstance(), &mVideoWindow::sigViewportDisplayChanged,
            this, &mPostCompVideoPanel::slot_viewportDisplayActiveChanged);
        connect(_checkBox_fillViewport, &QCheckBox::toggled,
            this, &mPostCompVideoPanel::slot_fillViewportToggled);

        //初始态：未导入视频，叠加/独立视口两形态开关禁用（勾了是空画面黑屏）
        refreshOverlayControls();
    }

    void mPostCompVideoPanel::newCompVideoPanel(int caseId)
    {
        //initReportPanel();

        ////创建曲线节点
        //MechCommon::mMechGlobalSignals::getInstance()->createExportWordSig();

        //updateExportWordPanel(caseid);

        //updateExportWordGUI();

    }

    void mPostCompVideoPanel::OnVideoFpsReady(double fps)
    {
        _spinBox_ratenum->setValue(fps);
    }

    void mPostCompVideoPanel::initialUI()
    {
        _checkBox_loopPlay->setChecked(true);
    }

    void mPostCompVideoPanel::slot_changeLanguage(int language)
    {
        if (language == 0)
        {
            _pushButton_importVideo->setText(QString::fromUtf8("导入视频"));
            slot_playStateChanged(mVideoWindow::getInstance()->isPlaying());
        }
        else if (language == 1)
        {
            _pushButton_importVideo->setText(QString::fromUtf8("Import Video"));
            slot_playStateChanged(mVideoWindow::getInstance()->isPlaying());
        }
    }

    void mPostCompVideoPanel::slot_importVideo()
    {
        QString caption = (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            ? QString::fromUtf8("Select video file")
            : QString::fromUtf8("选择视频文件");
        //FFmpeg 解码能力远不止 mp4，过滤放宽（avi/mkv/mov 等常见封装一并放行）
        QString filter = (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            ? QString::fromUtf8("Video files (*.mp4 *.avi *.mkv *.mov *.flv *.wmv *.m4v);;All files (*)")
            : QString::fromUtf8("视频文件 (*.mp4 *.avi *.mkv *.mov *.flv *.wmv *.m4v);;所有文件 (*)");
        QString filename = QFileDialog::getOpenFileName(this, caption,
            qApp->applicationDirPath(), filter);
        if (filename.isEmpty())
            return;

        mVideoWindow* videoInstance = mVideoWindow::getInstance();
        if (videoInstance == nullptr)
            return;
        bool ok = videoInstance->openVideo(filename);
        if (!ok)
        {
            //原实现静默吞失败（setFile false 无提示），此处明确告知
            QString title = (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                ? QString::fromUtf8("Open video failed")
                : QString::fromUtf8("视频打开失败");
            QString msg = (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                ? QString::fromUtf8("Unsupported format or corrupted file:\n%1").arg(filename)
                : QString::fromUtf8("格式不支持或文件损坏：\n%1").arg(filename);
            QMessageBox::warning(this, title, msg);
            return;
        }
        _pushButton_playVideo->show();
        _videoLoaded = true;
        refreshPlayButtonState();
        refreshOverlayControls();   //导入成功启用叠加/独立视口两形态开关
        _lineEdit_videoPath->setText(filename);
        //原始帧率经 sigVideoFpsReady 异步回填（OnVideoFpsReady），此处不再直读缓存
        //（原 setFile 后立即 getVideoOriginalFps 存在队列信号未到读到旧值的竞态）
    }

    void mPostCompVideoPanel::slot_playVideo()
    {
        mVideoWindow* videoInstance = mVideoWindow::getInstance();
        if (videoInstance == nullptr)
            return;
        //同步态视频由动画时间轴驱动，点击空转（按钮已禁用，此处双保险拦截重入）；
        //叠加/背景/独立视口/未选形态均可手动播放——dock 取消后控制面回归本按钮
        //（独立视口形态另有随本体的格子控制条）
        if (_checkBox_syncAnim->isChecked())
            return;
        //帧速率/循环播放均已改为变化即下发（构造期 connect），播放时无需重读
        videoInstance->togglePlay();
    }

    void mPostCompVideoPanel::refreshPlayButtonState()
    {
        //已导入 且 未勾同步（视频由动画时间轴驱动，点击空转）——播放按钮可用。
        //dock 取消后本按钮是叠加/背景/未选形态的唯一手动控制面（独立视口另有
        //随本体的格子控制条）
        _pushButton_playVideo->setEnabled(
            _videoLoaded
            && !_checkBox_syncAnim->isChecked());
    }

    void mPostCompVideoPanel::slot_syncToggled(bool checked)
    {
        mVideoWindow* videoInstance = mVideoWindow::getInstance();
        if (videoInstance == nullptr)
            return;
        videoInstance->setSyncMapping(_comboBox_syncMapping->currentIndex());
        videoInstance->setSyncOffset(_doubleSpinBox_syncOffset->value());
        videoInstance->setSyncWithAnimation(checked);
        //可用态统一收口（视频设置组随同步置灰等；dock 已取消无显隐矩阵）
        refreshOverlayControls();
        refreshPlayButtonState();
    }

    void mPostCompVideoPanel::slot_syncMappingChanged(int index)
    {
        if (mVideoWindow* videoInstance = mVideoWindow::getInstance())
            videoInstance->setSyncMapping(index);
    }

    void mPostCompVideoPanel::slot_syncOffsetChanged(double val)
    {
        if (mVideoWindow* videoInstance = mVideoWindow::getInstance())
            videoInstance->setSyncOffset(val);
    }

    void mPostCompVideoPanel::slot_playStateChanged(bool playing)
    {
        if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            _pushButton_playVideo->setText(playing ? QString::fromUtf8("Pause") : QString::fromUtf8("Play Video"));
        else
            _pushButton_playVideo->setText(playing ? QString::fromUtf8("暂停视频") : QString::fromUtf8("播放视频"));
    }

    void mPostCompVideoPanel::slot_overlayToggled(bool checked)
    {
        mVideoWindow* videoInstance = mVideoWindow::getInstance();
        if (videoInstance == nullptr)
            return;
        //参数先行下发（attach 即按当前参数定位；背景意图先于 attach 下发，
        //attach 时按其应用渲染背景透明与垫底层级）
        videoInstance->setOverlayPosition(_comboBox_overlayPos->currentIndex());
        videoInstance->setOverlayScalePct(_spinBox_overlayScale->value());
        videoInstance->setOverlayOpacityPct(_spinBox_overlayOpacity->value());
        videoInstance->setOverlayAsBackground(_checkBox_overlayBg->isChecked());
        videoInstance->setOverlayToViewport(checked);
        if (checked)
        {
            //叠加默认直达对比主用例（用户裁定）：自动勾"作为背景"+"与动画同步"
            //（setChecked 幂等——已勾不发信号；用户随后可手动取消，下次再勾不覆盖
            //用户已取消的选择之外的场景，保持简单恒勾）
            if (!_checkBox_overlayBg->isChecked())
                _checkBox_overlayBg->setChecked(true);
            if (!_checkBox_syncAnim->isChecked())
                _checkBox_syncAnim->setChecked(true);

            //自独立视口形态切入时，setOverlayToViewport 调用栈内已同步跑完
            //"关视频格→恢复云图激活"的整条联动（参数栏被挂云图面板、菜单切到
            //云图页）——此处拉回对比视频上下文：参数栏重挂本面板 + 菜单落
            //"结果"页。无可用后处理视口时开关回弹（isOverlayActive=false），
            //不动作。switchMenuBarWindow 受"树菜单栏联动"设置约束：None 档
            //仅重挂面板、菜单停留当前页
            if (videoInstance->isOverlayActive())
            {
                mPostInfoHub::getInstance()->editCompVideo();
                MechCommon::mMechGlobalSignals::getInstance()->switchMenuBarWindow(QString::fromUtf8("结果"));
            }
        }
        refreshOverlayControls();

        //dock 已取消：无显隐矩阵，可用态统一收口
        refreshPlayButtonState();
    }

    void mPostCompVideoPanel::slot_overlayBgToggled(bool checked)
    {
        mVideoWindow* videoInstance = mVideoWindow::getInstance();
        if (videoInstance == nullptr)
            return;
        videoInstance->setOverlayAsBackground(checked);
        refreshOverlayControls();
    }

    void mPostCompVideoPanel::refreshOverlayControls()
    {
        //全面板可用态收口（用户裁定：未导入视频前所有功能不可用）：
        //未导入 → 视频设置/动画同步/叠加组/铺满全部禁用（仅"导入视频"可用）；
        //同步态 → 帧速率/循环置灰（节奏随动画）；叠加总开关 → 全组；背景模式
        //（视频铺满）→ 位置/缩放无效；铺满钮随独立视口态（其余形态恒比例保持）
        const bool loaded = _videoLoaded;
        const bool sync = _checkBox_syncAnim->isChecked();
        const bool on = _checkBox_overlay->isChecked();
        const bool bg = _checkBox_overlayBg->isChecked();
        const bool vp = _checkBox_viewportDisplay->isChecked();
        //视频设置组
        _spinBox_ratenum->setEnabled(loaded && !sync);
        _checkBox_loopPlay->setEnabled(loaded && !sync);
        //动画同步组
        _checkBox_syncAnim->setEnabled(loaded);
        _comboBox_syncMapping->setEnabled(sync && loaded);
        _doubleSpinBox_syncOffset->setEnabled(sync && loaded);
        //叠加显示组（含独立视口/铺满）
        _checkBox_overlay->setEnabled(loaded);
        _checkBox_viewportDisplay->setEnabled(loaded);
        _checkBox_overlayBg->setEnabled(on && loaded);
        _comboBox_overlayPos->setEnabled(on && !bg && loaded);
        _spinBox_overlayScale->setEnabled(on && !bg && loaded);
        _spinBox_overlayOpacity->setEnabled(on && loaded);
        _checkBox_fillViewport->setEnabled(vp && loaded);
    }

    void mPostCompVideoPanel::slot_overlayPosChanged(int index)
    {
        if (mVideoWindow* videoInstance = mVideoWindow::getInstance())
            videoInstance->setOverlayPosition(index);
    }

    void mPostCompVideoPanel::slot_overlayScaleChanged(int val)
    {
        if (mVideoWindow* videoInstance = mVideoWindow::getInstance())
            videoInstance->setOverlayScalePct(val);
    }

    void mPostCompVideoPanel::slot_overlayOpacityChanged(int val)
    {
        if (mVideoWindow* videoInstance = mVideoWindow::getInstance())
            videoInstance->setOverlayOpacityPct(val);
    }

    void mPostCompVideoPanel::slot_overlayActiveChanged(bool active)
    {
        //叠加件被动失效（宿主视口关闭/无可用视口）：勾选与关联控件同步回弹，
        //blockSignals 防止回弹再触发 setOverlayToViewport
        if (_checkBox_overlay->isChecked() != active)
        {
            _checkBox_overlay->blockSignals(true);
            _checkBox_overlay->setChecked(active);
            _checkBox_overlay->blockSignals(false);
        }
        refreshOverlayControls();
        refreshPlayButtonState();   //叠加被动失效（视口关闭）回弹后，播放按钮可用态随之恢复
    }

    void mPostCompVideoPanel::slot_viewportDisplayToggled(bool checked)
    {
        mVideoWindow* videoInstance = mVideoWindow::getInstance();
        if (videoInstance == nullptr)
            return;
        //互斥（开视口自动关叠加）由 mVideoWindow::setViewportDisplay 内部完成，
        //叠加勾选经其 sigOverlayActiveChanged 链自动回弹，此处只下发本开关
        videoInstance->setViewportDisplay(checked);
        refreshOverlayControls();
        refreshPlayButtonState();
    }

    void mPostCompVideoPanel::slot_viewportDisplayActiveChanged(bool on)
    {
        //格子被外部删除回落隐藏态 / 反向互斥（勾叠加自动关格子）：勾选回弹
        if (_checkBox_viewportDisplay->isChecked() != on)
        {
            _checkBox_viewportDisplay->blockSignals(true);
            _checkBox_viewportDisplay->setChecked(on);
            _checkBox_viewportDisplay->blockSignals(false);
        }
        refreshOverlayControls();
        refreshPlayButtonState();
    }

    void mPostCompVideoPanel::slot_fillViewportToggled(bool checked)
    {
        //铺满仅独立视口形态生效（mVideoWindow 内按形态合成）；勾选态跨形态保留
        if (mVideoWindow* videoInstance = mVideoWindow::getInstance())
            videoInstance->setViewportFill(checked);
    }

}
