#pragma once
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

#include <memory>
#include <set>
#include <atomic>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QTreeWidget>
#include <QToolBar>
#include <QStyledItemDelegate>
#include <QApplication>
#include <QStyle>
#include <QMutex>
#include <QWaitCondition>
#include <QImage>
#include <QPointer>
#include <QStyleOptionViewItem>
#include <QPainter>
#include <QTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QTableWidget>

#include "mDockBase.h"
#include "mModelStruct.h"

#include <QObject>
#include <QVector2D>
#include <QVector3D>
#include <QMatrix4x4>
#include <QString>
#include <QThread>
#include <vector>
#include <functional>

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


namespace MViewBasic
{
    class mViewBase;
}
namespace mxr
{
    class REBUILD_PostRend;   // 叠加宿主视口（背景模式渲染开关还原用，QPointer 成员）
}
struct AVFrame;
struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVPacket;
struct SwsContext;

namespace PostWindow
{
//【ODR防御】收进PostWindow命名空间：此类曾有同名全局作用域拷贝编入MainWindow库（已删除），
//全局符号在多库场景下有Linux符号插入导致vtable错位的隐患
//
//【2026-11-16 对比视频重构·传输模型】自主时钟单循环改为命令驱动状态机：
//- GUI 线程经 play/pause/rewind/requestFrameAtTime 下发命令（m_cmdMutex + m_cmdWait 唤醒）；
//   所有 FFmpeg 上下文仅在解码线程内访问（setFile 于线程启动前、cleanFFmpeg 于线程停止后）。
//- requestFrameAtTime 为"拉模式"取帧：解出覆盖目标时刻的帧后即停——动画同步
//   （postCloudFrameChanged → 视频时间映射）逐帧驱动走此通道，播放/暂停/步进/
//   拖动/循环全部由动画侧事件天然覆盖，视频侧不再自持播放时钟。
//- 前向小步优化：目标时刻距当前帧 <1s 时免 seek 顺序解码，动画顺序推进时高频命中。
class DecodeThread : public QThread
{
    Q_OBJECT
public:
    explicit DecodeThread(QObject* parent = nullptr);
    ~DecodeThread() override;

    bool setFile(const QString& path);      // 打开/切换文件（须在线程未运行时调用）
    void stopDecode();                       // 请求线程退出（调用方负责 wait）

    // —— 传输控制（线程安全）——
    void play();                             // 继续播放（EOF 处自动回绕到开头）
    void pause();                            // 暂停（保留当前解码位置）
    void togglePlay();
    void rewind();                           // 停止并回绕：serve 首帧后进入暂停态
    void requestFrameAtTime(double sec);     // 拉模式：解出覆盖 sec 的帧后停（未处理的新请求覆盖旧请求）
    bool isPlaying() const;

    void setForceFps(double fps);            // >0 强制帧率（播放中生效）；<=0 用文件原始帧率
    double getForceFps() const;
    void setLoopPlay(bool enable);
    bool isLoopPlay() const;

    double getOriginalFps() const;           // 文件原始帧率（打开后有效）
    double getDurationSec() const;           // 总时长（秒）

signals:
    void sigNewFrame(const QImage& img, double ptsSec);    // 帧到达（携带帧时间，供进度条/同步显示）
    void sigVideoFpsReady(double fps);
    void sigStreamInfo(double durationSec);
    void sigPlayFinished();                  // 非循环自然结束
    void sigPlayStateChanged(bool playing);  // 播放态变化（控制条/面板按钮镜像）

protected:
    void run() override;

private:
    enum class Transport
    {
        Idle,       // 未播放（线程运行中但无命令）
        Paused,     // 暂停/停驻：等待命令
        Playing     // 自主播放（按帧率/pts 节奏推进）
    };

    // 解出一帧（跳过非视频包，含 EAGAIN 续读）；EOF/错误返回 false
    bool decodeNextFrame(double& ptsSec, QImage* outImg = nullptr);
    // 定位到覆盖 sec 的帧并显示（seek BACKWARD + 前向解码；前向小步免 seek）
    bool serveFrameAt(double sec);
    // 当前已解码帧（m_frameRaw）转 RGB 发出
    void emitFrame(double ptsSec);
    // 播放节奏等待到 targetUs；中途暂停/退出请求则提前返回 false
    bool sleepUntil(int64_t targetUs);
    void cleanFFmpeg();
    bool seekToStart();

private:
    QString m_filePath;

    // 命令状态（m_cmdMutex 保护；wait 条件 = Playing / 有 seek 请求 / 退出）
    mutable QMutex m_cmdMutex;
    QWaitCondition m_cmdWait;
    Transport m_transport = Transport::Idle;
    bool m_exitThread = false;
    bool m_seekRequest = false;
    double m_seekTargetSec = 0.0;
    bool m_loopPlay = false;
    double m_forceFps = -1.0;
    std::atomic<bool> m_atEof{ false };      // 解码线程写、play() 读：EOF 后再 play 需先回绕

    // FFmpeg 上下文：仅解码线程访问（setFile 于线程启动前、cleanFFmpeg 于线程停止后）
    AVFormatContext* m_fmtCtx = nullptr;
    AVCodecContext* m_codecCtx = nullptr;
    const AVCodec* m_codec = nullptr;
    AVFrame* m_frameRaw = nullptr;
    AVFrame* m_frameRgb = nullptr;
    AVPacket* m_pkt = nullptr;
    SwsContext* m_swsCtx = nullptr;
    int m_videoStreamIndex = -1;
    uint8_t* m_rgbBuffer = nullptr;

    double m_originalFps = 0.0;      // 文件原始帧率
    double m_durationSec = 0.0;      // 总时长（秒）
    double m_lastPtsSec = -1.0;      // 最近已显示帧时间（解码线程私有）
};
} // namespace PostWindow


namespace PostWindow
{
    //视频画面画布：缓存缩放 + 居中绘制。
    //（修原缺陷：先 KeepAspectRatio 缩放再 drawImage(this->rect()) 把留边小图拉伸回
    // 全窗，比例保持白做导致画面变形；且每帧 paint 实时 SmoothTransformation 开销大）
    class mVideoCanvas : public QWidget
    {
        Q_OBJECT
    public:
        explicit mVideoCanvas(QWidget* parent = nullptr);

        void setFrame(const QImage& img);      // GUI 线程调用（帧经队列信号到达，无跨线程竞争）
        void clearFrame();
        void setFillViewport(bool fill);       // true=拉伸铺满（独立视口形态，无视宽高比）；false=KeepAspectRatio

        bool fillViewport() const { return m_fillViewport; }

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        QImage m_frame;                 // 原始帧（视频分辨率）
        QImage m_scaled;                // 缩放缓存（按当前模式）
        QSize m_scaledForSize;          // 缓存对应的画布尺寸（尺寸变化即失效）
        bool m_dirty = true;            // 帧更新即失效
        bool m_fillViewport = false;    // 铺满模式（缓存失效判据之一）
    };

    //视口叠加画布：视频以可调透明度叠在动画视口渲染表面之上（HyperView overlay 语义）。
    //挂载=渲染表面（osg/QOpenGLWidget）父件的同级覆盖件并 raise——GL 子件会被纹理盖住，
    //Qt5 已知限制，同 mPostQueryResultsDock 先例；点击穿透不挡视口相机操作；
    //半透明经 WA_TranslucentBackground + painter 透明度（mAnimationRecordWidget 覆盖件先例）。
    class mVideoOverlayCanvas : public QWidget
    {
        Q_OBJECT
    public:
        explicit mVideoOverlayCanvas(QWidget* parent = nullptr);

        void setFrame(const QImage& img);
        void clearFrame();
        void setOpacityPct(int pct);         // 10~100

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        QImage m_frame;
        QImage m_scaled;
        QSize m_scaledForSize;
        bool m_dirty = true;
        qreal m_opacity = 1.0;
    };

    /*
    * 视频窗口本体（dock 已取消：独立视口格承载或隐藏待选）：画布 + 播放控制条（播放/暂停、停止、进度条、时间）。
    * 动画同步模式（HyperView video sync 语义）：消费全局 postCloudFrameChanged(实际帧号)，
    * 帧号→激活视口 postDB.getFrameTime → 仿真时间，按映射模式（1:1 对齐/全程映射+偏移）
    * 换算视频时间后走拉模式取帧；同步期间自主播放控制禁用。
    */
    class MPOSTWINDOW_EXPORT mVideoWindow : public QWidget
    {
        Q_OBJECT
    public:
        static mVideoWindow* getInstance();

        explicit mVideoWindow(QWidget* parent = nullptr);
        ~mVideoWindow() override;

        bool openVideo(const QString& filePath);
        void closeVideo();

        void play();                       // 开始/继续播放
        void pause();                      // 暂停
        void togglePlay();
        void stop();                       // 停止并回绕到首帧
        bool isPlaying() const;
        void seekToTime(double sec);       // 手动定位（进度条拖动）

        // 对外接口
        double getVideoOriginalFps() const;
        double getVideoDuration() const;
        void setPlayFps(double fps);       // 设置播放帧率，<=0 恢复原始（播放中生效）
        void setLoopPlay(bool loop);
        bool isLoopPlay() const;

        // —— 动画同步 ——
        void setSyncWithAnimation(bool on);   // 开启后视频由动画换帧事件驱动
        bool isSyncWithAnimation() const;
        void setSyncMapping(int mode);         // 0=按时间对齐（秒对秒） 1=全程映射（起止对齐）
        void setSyncOffset(double sec);        // 视频时间偏移（校准实验视频与仿真起点）

        // —— 视口叠加（HyperView overlay：视频半透明叠在动画视口上）——
        void setOverlayToViewport(bool on);    // 叠加到激活后处理视口；关闭摘除叠加件（本体隐藏待选形态）
        bool isOverlayToViewport() const;      // 用户意图开关态（实际叠加件存活见 isOverlayActive）
        bool isOverlayActive() const;          // 叠加件当前是否在视口上（视口关闭会被动失效）
        void setOverlayPosition(int pos9);     // 9宫格 0..8（行优先 左上..右下，4=居中）
        void setOverlayScalePct(int pct);      // 画面占视口比例 10~100
        void setOverlayOpacityPct(int pct);    // 不透明度 10~100

        // —— 背景模式（对比视频本义：视频作动画背景）——
        // 视频帧画进场景背景 quad（渲染侧纹理路径 mBaseRend::updateVideoBackgroundFrame，
        // GL 内部完成不依赖 Qt 窗口合成）；关闭还原渐变背景
        void setOverlayAsBackground(bool on);
        bool isOverlayAsBackground() const;

        // —— 独立视口形态（图表卡片同款：mVideoWindow 本体整体进驻视口矩阵
        // 格子，经全局信号由主窗口代办建/关格；与叠加互斥，关格回落隐藏待选形态）——
        void setViewportDisplay(bool on);
        bool isViewportDisplay() const;
        //铺满模式（仅独立视口形态生效）：true=拉伸铺满格子无视宽高比，
        //false=保持宽高比居中（黑边）；叠加/隐藏形态恒为比例保持
        void setViewportFill(bool fill);
        bool isViewportFill() const;

    signals:
        void sigVideoFpsReady(double fps);     // 视频原始帧率就绪（替代原 AddFpsReadyFunc 回调注册——
                                               // 单例存裸 this bind 回调在面板销毁后悬空，改信号自动断连）
        void sigPlayStateChanged(bool playing);
        void sigPlayFinished();
        void sigOverlayActiveChanged(bool active);   // 叠加实际存活态变化（视口关闭被动失效时回弹面板勾选）
        void sigViewportDisplayChanged(bool on);     // 独立视口形态变化（格子被外部删除回落时回弹面板勾选）

    private slots:
        void onNewFrame(const QImage& frameImg, double ptsSec);
        void onThreadFpsReady(double fps);
        void onThreadStreamInfo(double durationSec);
        void onThreadPlayFinished();
        void onThreadPlayStateChanged(bool playing);
        void onCloudFrameChanged(int frame);     // 动画/云图换帧全局通知 → 同步模式驱动视频
        void onSliderMoved(int value);
        void onPlayClicked();
        void onStopClicked();
        void slot_changeLanguage(int language);
        void onVideoViewportClosed();          // 视口格子被外部删除（全局通知）→ 状态复位+面板回弹

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;   // 渲染表面/其父件 Resize → 叠加画布重排

    private:
        //激活后处理视口 → postDB，帧号→仿真时间；同时刷新动画时间范围缓存（按 postname）
        bool resolveSimTime(int frame, double& simTime);
        double mapToVideoTime(double simTime) const;
        //当前后处理模型的云图视口（用户口径：一个模型只有一个视口）——经
        //activatePostViewport（当前派生优先、激活悬空时按类型遍历兜底），不依赖
        //激活态：前处理视口激活/关视频格激活悬空时 getPostRenderWidget 派生空，
        //叠加/同步/背景解析出错即此
        mxr::REBUILD_PostRend* cloudRendOfCurrentModel();
        //同步参数变化后按激活视口当前帧立即重对齐（调偏移时动画常暂停，无换帧事件则无反馈）
        void reAlignToCurrentFrame();
        //叠加件生命周期：attach 挂激活视口渲染表面父件（换视口=detach+attach）
        void attachOverlay();
        void detachOverlay();
        void updateOverlayGeometry();      // 9宫格定位 + 帧宽高比 + 缩放百分比（父件坐标系）
        //背景模式推帧：QImage(RGB888)→紧凑行主序 RGB24→宿主视口背景纹理
        //（画布路径 vs 纹理路径在此分流；QImage bytesPerLine 有 4 字节对齐
        //padding，纹理契约要求每行 w*3 紧凑，须逐行拷贝）
        void pushBackgroundFrame(const QImage& img);
        void clearBackgroundFrame();       //背景模式退出：还原宿主视口渐变背景
        void refreshTimeLabel();
        void updateControlEnabled();      // 无视频/同步模式下的控制条可用态
        void refreshButtonTexts();

    private:
        DecodeThread* m_decodeThread = nullptr;

        mVideoCanvas*  m_canvas = nullptr;          // 画面画布
        QPushButton*  m_btnPlay = nullptr;          // 播放/暂停
        QPushButton*  m_btnStop = nullptr;          // 停止（暂停+回绕）
        QSlider*      m_slider = nullptr;           // 进度（毫秒粒度）
        QLabel*       m_labelTime = nullptr;        // 当前时间/总时长
        bool          m_sliderDragging = false;

        double m_cacheOriginalFps = 0.0;
        double m_durationSec = 0.0;
        double m_lastPtsSec = 0.0;
        bool m_isPlaying = false;
        bool m_hasVideo = false;           // 文件打开成功即真（duration 可能是估算值，不作为打开判据）

        // —— 动画同步（GUI 线程私有）——
        bool m_syncWithAnim = false;
        int m_syncMapping = 0;              // 0=按时间对齐 1=全程映射
        double m_syncOffset = 0.0;
        QString m_aniPostName;              // 动画时间范围来源（换结果文件失效重查）
        bool m_aniRangeValid = false;
        double m_aniStartTime = 0.0;
        double m_aniEndTime = 0.0;

        // —— 视口叠加（GUI 线程私有）——
        QPointer<mVideoOverlayCanvas> m_overlay;   // 视口关闭随宿主销毁→QPointer 置空（安全判空）
        QPointer<QWidget> m_overlaySurface;  // 渲染表面（几何基准 + Resize 监听）
        QPointer<mxr::REBUILD_PostRend> m_overlayRend;  // 叠加宿主视口（背景模式渲染开关还原用）
        bool m_overlayOn = false;            // 用户意图开关
        bool m_overlayAsBackground = false;  // 背景模式：视频铺满垫底 + 渲染背景透明
        bool m_viewportDisplay = false;      // 独立视口形态（与叠加互斥）
        bool m_viewportFill = true;          // 独立视口铺满模式（用户可选，默认铺满）
        int m_overlayPos9 = 4;               // 9宫格（4=居中）
        int m_overlayScalePct = 40;          // 占视口比例
        int m_overlayOpacityPct = 80;        // 不透明度
        double m_frameAspect = 0.0;          // 帧宽高比（首帧采集，几何用）
        QImage m_lastFrameImg;               // 最近帧缓存（背景模式开启/参数变化时重推）

    protected:
        static mVideoWindow* _instance;

    };
}
